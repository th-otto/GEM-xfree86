/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_bios.c,v 1.3 2000/02/18 12:20:00 tsi Exp $ */

#include "xf86.h"
#include "xf86PciInfo.h"

#include "sis.h"
#include "sis_regs.h"
#include "sis_bios.h"


static	unsigned short	ModeIDOffset, FRateOffset,
			CRTC1Offset, VCLKOffset;
static	unsigned char * CRTC2Ptr;
static	unsigned short	VBInfo, SetFlag, ModeType;
static	unsigned char	LCDResInfo, LCDTypeInfo, LCDInfo;
static	unsigned short	RVBHRS;
static	unsigned short	HDE, VGAHDE, VDE, VGAVDE;
static	unsigned short	HT, VGAHT, VT, VGAVT;
static	unsigned char	NewFlickerMode, RY1COE, RY2COE, RY3COE, RY4COE;
static	unsigned short	RVBHCMAX;
static	unsigned char	RVBHCFACT;








			

static	unsigned short	StResInfo[][2] = {{640,400},{640,350},{720,400},
					  {720,350},{640,480}};
static	unsigned short	ModeResInfo[][4] = {
			{ 320, 200,8, 8},{ 320, 240,8, 8},{ 320, 400,8, 8},
			{ 400, 300,8, 8},{ 512, 384,8, 8},{ 640, 400,8,16},
			{ 640, 480,8,16},{ 800, 600,8,16},{1024, 768,8,16},
			{1280,1024,8,16},{1600,1200,8,16},{1920,1440,8,16},
			{ 720, 480,8,16},{ 720, 576,8,16},{2048,1536,8,16}};

static	unsigned char	StLCD1Data[][8] = {
				LCDDATA(66, 31, 992, 510, 1320, 816),
				LCDDATA(66, 31, 992, 510, 1320, 816),
				LCDDATA(176, 75, 900, 510, 1320, 816),
				LCDDATA(176, 75, 900, 510, 1320, 816),
				LCDDATA(66, 31, 992, 510, 1320, 816),
				LCDDATA(27, 16, 1024, 650, 1350, 832),
				LCDDATA(1, 1, 1344, 806, 1344, 806)};
static	unsigned char	ExtLCD1Data[][8] = {
				LCDDATA(12,5,896,512,1344,806),
				LCDDATA(12,5,896,510,1344,806),
				LCDDATA(32,15,1008,505,1344,806),
				LCDDATA(32,15,1008,514,1344,806),
				LCDDATA(12,5,896,500,1344,806),
				LCDDATA(42,25,1024,625,1344,806),
				LCDDATA(1,1,1344,806,1344,806),
				LCDDATA(12,5,896,500,1344,806),
				LCDDATA(42,25,1024,625,1344,806),
				LCDDATA(1,1,1344,806,1344,806),
				LCDDATA(12,5,896,500,1344,806),
				LCDDATA(42,25,1024,625,1344,806),
				LCDDATA(1,1,1344,806,1344,806)};

static	unsigned char	StLCD2Data[][8] = {
				LCDDATA(4,1,880,510,1650,1088),
				LCDDATA(4,1,880,510,1650,1088),
				LCDDATA(176,45,900,510,1650,1088),
				LCDDATA(176,45,900,510,1650,1088),
				LCDDATA(4,1,880,510,1650,1088),
				LCDDATA(13,5,1024,675,1560,1152),
				LCDDATA(16,9,1266,804,1688,1072),
				LCDDATA(1,1,1688,1066,1688,1066)};
static	unsigned char	ExtLCD2Data[][8] = {
				LCDDATA(211,60,1024,501,1688,1066),
				LCDDATA(211,60,1024,508,1688,1066),
				LCDDATA(211,60,1024,501,1688,1066),
				LCDDATA(211,60,1024,508,1688,1066),
				LCDDATA(211,60,1024,500,1688,1066),
				LCDDATA(211,75,1024,625,1688,1066),
				LCDDATA(211,120,1280,798,1688,1066),
				LCDDATA(1,1,1688,1066,1688,1066)};

static	unsigned char	StPALData[][16] = {
		TVDATA(1,1,864,525,1270,400,100,0,760,0xF4,0xFF,0x1C,0x22),
		TVDATA(1,1,864,525,1270,350,100,0,760,0xF4,0xFF,0x1C,0x22),
		TVDATA(1,1,864,525,1270,400,0,0,720,0xF4,0x0B,0x1C,0x0A),
		TVDATA(1,1,864,525,1270,350,0,0,720,0xF4,0x0B,0x1C,0x0A),
		TVDATA(1,1,864,525,1270,480,50,0,760,0xF4,0xFF,0x1C,0x22),
		TVDATA(1,1,864,525,1270,600,50,0,0,0xF4,0xFF,0x1C,0x22)};
static	unsigned char	ExtPALData[][16] = {
		TVDATA(27,10,848,448,1270,530,50,0,50,0xF4,0xFF,0x1C,0x22),
		TVDATA(108,35,848,398,1270,530,50,0,50,0xF4,0xFF,0x1C,0x22),
		TVDATA(12,5,954,448,1270,530,50,0,50,0xF4,0x0B,0x1C,0x0A),
		TVDATA(9,4,960,463,1644,438,50,0,50,0xF4,0x0B,0x1C,0x0A),
		TVDATA(9,4,848,528,1270,530,0,0,50,0xF5,0xFB,0x1B,0x2A),
		TVDATA(36,25,1060,648,1316,530,438,0,438,0xEB,0x05,0x25,0x16),
		TVDATA(3,2,1080,619,1270,540,438,0,438,0xF3,0x02,0x1D,0x1C)};

static	unsigned char	StNTSCData[][16] = {
		TVDATA(1,1,858,525,1270,400,50,0,760,0xF1,0x04,0x1F,0x18),
		TVDATA(1,1,858,525,1270,350,50,0,640,0xF1,0x04,0x1F,0x18),
		TVDATA(1,1,858,525,1270,400,0,0,720,0xF4,0x0B,0x1C,0x0A),
		TVDATA(1,1,858,525,1270,350,0,0,720,0xF4,0x0B,0x1C,0x0A),
		TVDATA(1,1,858,525,1270,480,0,0,760,0xF1,0x04,0x1F,0x18)};
static	unsigned char	ExtNTSCData[][16] = {
		TVDATA(143,65,858,443,1270,440,171,0,171,0xF1,0x04,0x1F,0x18),
		TVDATA(88,35,858,393,1270,440,171,0,171,0xF1,0x04,0x1F,0x18),
		TVDATA(143,70,924,443,1270,440,92,0,92,0xF4,0x0B,0x1C,0x0A),
		TVDATA(143,70,924,393,1270,440,92,0,92,0xF4,0x0B,0x1C,0x0A),
		TVDATA(143,76,836,523,1270,440,224,0,0,0xF1,0x05,0x1F,0x16),
		TVDATA(143,120,1056,643,1288,440,0,1,0,0xF4,0x10,0x1C,0x00),
		TVDATA(143,76,836,523,1270,440,0,1,0,0xE7,0x0E,0x29,0x04)};

static	unsigned char	StHiTVData[][12] = {
				HITVDATA(1,1,892,563,720,800,0,0,0),
				HITVDATA(1,1,892,563,720,700,0,0,0),
				HITVDATA(1,1,892,563,721,800,0,0,0),
				HITVDATA(1,1,892,563,720,700,0,0,0),
				HITVDATA(1,1,892,563,720,960,0,0,0),
				HITVDATA(10,3,1008,625,1632,960,0x143,1,0),
				HITVDATA(25,12,1260,851,1632,960,0x032,0,0),
				HITVDATA(5,4,1575,1124,1632,960,0x128,0,0)};
static	unsigned char	ExtHiTVData[][12] = {
				HITVDATA(3,1,840,563,1632,960,0,0,0),
				HITVDATA(3,1,960,563,1632,960,0,0,0),
				HITVDATA(3,1,840,483,1632,960,0,0,0),
				HITVDATA(3,1,960,563,1632,960,0,0,0),
				HITVDATA(3,1,1400,563,1632,960,0x166,1,0),
				HITVDATA(10,3,1008,625,1632,960,0x143,1,0),
				HITVDATA(25,12,1260,851,1632,960,0x032,0,0),
				HITVDATA(5,4,1575,1124,1632,960,0x128,0,0)};

/* for group2 */
static	unsigned char	HiTVTiming1[] = {
			0x20,0x53,0x2C,0x5F,0x08,0x31,0x3A,0x64,
			0x28,0x02,0x01,0x3D,0x06,0x3E,0x35,0x6D,
			0x06,0x14,0x3E,0x35,0x6D,0x00,0xC5,0x3F,
			0x64,0x90,0x33,0x8C,0x18,0x36,0x1B,0x13,
			0x2A,0xDE,0x2A,0x44,0x40,0x2A,0x44,0x40,
			0x8E,0x8E,0x8F,0x07,0x0B};
static	unsigned char	HiTVTiming2[] = {
			0x92,0x0F,0x40,0x60,0x80,0x14,0x90,0x8C,
			0x60,0x14,0x1A,0xB0,0x4F};
static	unsigned short	HiTVTiming3[] = {0x0027,0xFFFC,0x006A};

static	unsigned char	HiTVTimingSimu1[] = {
			0x20,0x53,0x2C,0x5F,0x08,0x31,0x3A,0x65,
			0x28,0x02,0x01,0x3D,0x06,0x3E,0x35,0x6D,
			0x06,0x14,0x3E,0x35,0x6D,0x00,0xC5,0x3F,
			0x65,0x90,0x7B,0xA8,0x03,0xF0,0x78,0x03,
			0x11,0x15,0x11,0xCF,0x10,0x11,0xCF,0x10,
			0x35,0x35,0x3C,0x69,0x1D};
static	unsigned char	HiTVTimingSimu2[] = {
			0x92,0x0F,0x40,0x60,0x80,0x14,0x90,0x8C,
			0x60,0x04,0x77,0x72,0x5C};
static	unsigned short	HiTVTimingSimu3[] = {0x000E,0xFFFC,0x002D};

static	unsigned char	NTSCTiming1[] = {
			0x17,0x1D,0x03,0x09,0x05,0x06,0x0C,0x0C,
			0x94,0x49,0x01,0x0A,0x06,0x0D,0x04,0x0A,
			0x06,0x14,0x0D,0x04,0x0A,0x00,0x85,0x1B,
			0x0C,0x50,0x00,0x99,0x00,0xEC,0x4A,0x17,
			0x88,0x00,0x4B,0x00,0x00,0xE2,0x00,0x02,
			0x03,0x0A,0x65,0x9D,0x08};
static	unsigned char	NTSCTiming2[] = {
			0x92,0x8F,0x40,0x60,0x80,0x14,0x90,0x8C,
			0x60,0x14,0x50,0x00,0x40};
static	unsigned short	NTSCTiming3[] = {0x0044,0x02DB,0x003B};

static	unsigned char	PALTiming1[] = {
			0x19,0x52,0x35,0x6E,0x04,0x38,0x3D,0x70,
			0x94,0x49,0x01,0x12,0x06,0x3E,0x35,0x6D,
			0x06,0x14,0x3E,0x35,0x6D,0x00,0x45,0x2B,
			0x70,0x50,0x00,0x97,0x00,0xD7,0x5D,0x17,
			0x88,0x00,0x45,0x00,0x00,0xE8,0x00,0x02,
			0x0D,0x00,0x68,0xB0,0x0B};
static	unsigned char	PALTiming2[] = {
			0x92,0x8F,0x40,0x60,0x80,0x14,0x90,0x8C,
			0x60,0x14,0x63,0x00,0x40};
static	unsigned short	PALTiming3[] = {0x003E,0x02E1,0x0028};

/* for group3 */
static	unsigned char	HiTVGroup3Data[] = {
			0x00,0x1A,0x22,0x63,0x62,0x22,0x08,0x5F,
			0x05,0x21,0xB2,0xB2,0x55,0x77,0x2A,0xA6,
			0x25,0x2F,0x47,0xFA,0xC8,0xFF,0x8E,0x20,
			0x8C,0x6E,0x60,0x2E,0x58,0x48,0x72,0x44,
			0x56,0x36,0x4F,0x6E,0x3F,0x80,0x00,0x80,
			0x4F,0x7F,0x03,0xA8,0x7D,0x20,0x1A,0xA9,
			0x14,0x05,0x03,0x7E,0x64,0x31,0x14,0x75,
			0x18,0x05,0x18,0x05,0x4C,0xA8,0x01};
static	unsigned char	HiTVGroup3Simu[] = {
			0x00,0x1A,0x22,0x63,0x62,0x22,0x08,0x95,
			0xDB,0x20,0xB8,0xB8,0x55,0x47,0x2A,0xA6,
			0x25,0x2F,0x47,0xFA,0xC8,0xFF,0x8E,0x20,
			0x8C,0x6E,0x60,0x15,0x26,0xD3,0xE4,0x11,
			0x56,0x36,0x4F,0x6E,0x3F,0x80,0x00,0x80,
			0x67,0x36,0x01,0x47,0x0E,0x10,0xBE,0xB4,
			0x01,0x05,0x03,0x7E,0x65,0x31,0x14,0x75,
			0x18,0x05,0x18,0x05,0x4C,0xA8,0x01};
static	unsigned char	NTSCGroup3Data[] = {
			0x00,0x14,0x15,0x25,0x55,0x15,0x0B,0x89,
			0xD7,0x40,0xB0,0xB0,0xFF,0xC4,0x45,0xA6,
			0x25,0x2F,0x67,0xF6,0xBF,0xFF,0x8E,0x20,
			0x8C,0xDA,0x60,0x92,0xC8,0x55,0x8B,0x00,
			0x51,0x04,0x18,0x0A,0xF8,0x87,0x00,0x80,
			0x3B,0x3B,0x00,0xF0,0xF0,0x00,0xF0,0xF0,
			0x00,0x51,0x0F,0x0F,0x08,0x0F,0x08,0x6F,
			0x18,0x05,0x05,0x05,0x4C,0xAA,0x01};
static	unsigned char	PALGroup3Data[] = {
			0x00,0x1A,0x22,0x63,0x62,0x22,0x08,0x85,
			0xC3,0x20,0xA4,0xA4,0x55,0x47,0x2A,0xA6,
			0x25,0x2F,0x47,0xFA,0xC8,0xFF,0x8E,0x20,
			0x8C,0xDC,0x60,0x92,0xC8,0x4F,0x85,0x00,
			0x56,0x36,0x4F,0x6E,0xFE,0x83,0x54,0x81,
			0x30,0x30,0x00,0xF3,0xF3,0x00,0xA2,0xA2,
			0x00,0x48,0xFE,0x7E,0x08,0x40,0x08,0x91,
			0x18,0x05,0x18,0x05,0x4C,0xA8,0x01};




Bool
SiSSetMode(ScrnInfoPtr pScrn, unsigned short ModeNo)
{
	unsigned long   temp;
	unsigned char * ROMAddr  = SISPTR(pScrn)->BIOS;
	unsigned short  BaseAddr = SISPTR(pScrn)->RelIO;

	temp=SearchModeID(ROMAddr,ModeNo);	/* 2.Get ModeID Table */
	if(temp==0)  return(0);
   
	GetVBInfo(BaseAddr,ROMAddr);		/* add for CRT2 */
	ErrorF("VBInfo=0x%0X\nSetFlag=0x%0X\nModeType=0x%0X\n",
			VBInfo, SetFlag, ModeType);
	GetLCDResInfo(BaseAddr+CROFFSET);	/* add for CRT2 */

	if (VBInfo & (SET_SIMU_SCAN_MODE | SWITCH_TO_CRT2))
		SetFlag |= PROGRAMMING_CRT2;

	/* Skip Set CRT1 Regs */

	if ((VBInfo & SET_SIMU_SCAN_MODE) || (VBInfo & SWITCH_TO_CRT2))
		SetCRT2Group(pScrn,ModeNo);		/* add for CRT2 */

	return TRUE;
}

Bool
SearchModeID(unsigned char *ROMAddr, unsigned short ModeNo)
{
	unsigned char	ModeID;
	unsigned short	ModeIDLen;

	if (ModeNo <= 0x13)  {
		ModeIDOffset=*(unsigned short *)(ROMAddr+0x200);
		ModeIDLen = STMODE_SIZE;
	} else  {
		ModeIDOffset=*(unsigned short *)(ROMAddr+0x20A);
		ModeIDLen = EMODE_SIZE;
	}

	ModeID=*(ROMAddr+ModeIDOffset);
	while (ModeID!=0xff && ModeID!=ModeNo) {
		ModeIDOffset += ModeIDLen;
		ModeID=*(ROMAddr+ModeIDOffset);
	}
	if(ModeID==0xff)
		return(FALSE);
	else
		return(TRUE);
}

void
GetVBInfo(unsigned short BaseAddr, unsigned char *ROMAddr)
{
	unsigned short	flag;
	unsigned short	tempbx;
	unsigned short	tempbl,tempbh;
	unsigned short	p3c4 = BaseAddr + SROFFSET;
	unsigned short	p3d4 = BaseAddr + CROFFSET;
  
	SetFlag=0;
	tempbx = *(CARD16 *)(ROMAddr+ModeIDOffset+1);	/* si+St_ModeFlag */
	ModeType = tempbx & MODE_INFO_FLAG;

	inSISIDXREG(p3c4,0x38,flag);		/* call BridgeisOn */
	if(!(flag & 0x20)){
		VBInfo = 0;
		return;
	}
	inSISIDXREG(p3d4,0x30,tempbl);
	inSISIDXREG(p3d4,0x31,tempbh);
	tempbx = tempbh << 8 | tempbl;
	tempbx &= (~(SET_IN_SLAVE_MODE | DISABLE_CRT2_DISPLAY) );
	if(!(tempbx & 0x07C)) {
		VBInfo=SET_SIMU_SCAN_MODE | DISABLE_CRT2_DISPLAY;
		return;
	}
	if (tempbx & SET_CRT2_TO_RAMDAC)  {
		tempbx &= (0xFF00 | SET_CRT2_TO_RAMDAC | SWITCH_TO_CRT2 |
				SET_SIMU_SCAN_MODE);
	} else if (tempbx & SET_CRT2_TO_LCD)  {
		tempbx &= (0xFF00 | SET_CRT2_TO_LCD | SWITCH_TO_CRT2 |
				SET_SIMU_SCAN_MODE);
	} else if (tempbl & SET_CRT2_TO_SCART)  {
		tempbx &=(0xFF00 | SET_CRT2_TO_SCART |SWITCH_TO_CRT2 |
				SET_SIMU_SCAN_MODE);
	} else if (tempbl & SET_CRT2_TO_HIVISION_TV)  {
		tempbx &= (0xFF00 | SET_CRT2_TO_HIVISION_TV | SWITCH_TO_CRT2 |
				SET_SIMU_SCAN_MODE);
	}
	if ((tempbl & (DISABLE_CRT2_DISPLAY >> 8)) &&
	    (tempbl & (SWITCH_TO_CRT2 | SET_SIMU_SCAN_MODE))==0)
		tempbx = SET_SIMU_SCAN_MODE | DISABLE_CRT2_DISPLAY;

	if(!(tempbx & DRIVER_MODE))
		tempbx |= SET_SIMU_SCAN_MODE;

	if (!(tempbx & SET_SIMU_SCAN_MODE))  {
		if (tempbx & SWITCH_TO_CRT2)  {
			flag = *(CARD16 *)(ROMAddr+ModeIDOffset+1);
			if (!(flag & CRT2_MODE))
				tempbx |= SET_SIMU_SCAN_MODE;
		}  else  {
			if (BridgeIsEnable(BaseAddr) && BridgeInSlave(BaseAddr))
				tempbx |= SET_SIMU_SCAN_MODE;
		}
	}
	/* GetVBInfoCnt */
	if (tempbx & DISABLE_CRT2_DISPLAY)  {
		VBInfo = tempbx;
		return;
	}
	if (tempbx & DRIVER_MODE)  {
		if (!(tempbx & SET_SIMU_SCAN_MODE))  {
			VBInfo = tempbx;
			return;
		}
		flag = *(CARD16 *)(ROMAddr+ModeIDOffset+1);
		if (flag & CRT2_MODE)  {
			VBInfo = tempbx;
			return;
		}
	}
	tempbx |= SET_IN_SLAVE_MODE;
	if ((tempbx & SET_CRT2_TO_TV) && (!(tempbx & SET_NO_SIMU_TV_ON_LOCK)))
		SetFlag |= TV_SIMU_MODE;

	VBInfo = tempbx;
	return;
}

Bool
BridgeIsEnable(CARD16 BaseAddr)
{
	unsigned char	temp;

	if (!BridgeIsOn(BaseAddr))
		return	FALSE;
	inSISIDXREG(BaseAddr+P1_INDEX, 0, temp);
	if (temp & 0xA0)
		return TRUE;
	else
		return FALSE;
}

Bool
BridgeIsOn(CARD16 BaseAddr)
{
	unsigned char	temp;

	inSISIDXREG(BaseAddr+SROFFSET, 0x38, temp);
	if (temp & 0x20)
		return TRUE;
	else
		return FALSE;
}

Bool
BridgeInSlave(CARD16 BaseAddr)
{
	unsigned char	temp;

	inSISIDXREG(BaseAddr+CROFFSET, 0x31, temp);
	if (temp & SET_IN_SLAVE_MODE)
		return TRUE;
	else
		return FALSE;
}

void
GetLCDResInfo(unsigned short p3d4)
{
	unsigned short tempah,tempbh;	
  
	inSISIDXREG(p3d4, 0x36, tempbh);
	tempah = tempbh & 0x0F;

	if (tempah > PANEL_1280x1024)
		tempah = 0;
	LCDResInfo = tempah;
	LCDTypeInfo = tempbh >> 4;
	inSISIDXREG(p3d4, 0x37, LCDInfo);
	if(!(VBInfo & SET_CRT2_TO_LCD))
		return;
	if (!(VBInfo & (SET_SIMU_SCAN_MODE | SWITCH_TO_CRT2)))
		return;
	if (VBInfo & SET_IN_SLAVE_MODE)  {
		if (VBInfo & SET_NO_SIMU_ON_LOCK)
			SetFlag |= LCDVESA_TIMING;	
	} else
		SetFlag |= LCDVESA_TIMING;	
}

void
SetCRT2Group(ScrnInfoPtr pScrn, CARD16 ModeNo)
{
	unsigned short	temp;
	SISPtr		pSiS = SISPTR(pScrn);
	unsigned short	BaseAddr = pSiS->RelIO;
	unsigned char *	ROMAddr = pSiS->BIOS;
	
   
	SetFlag |= PROGRAMMING_CRT2;

	GetRatePtr(pScrn, ModeNo, SELECT_CRT2);
	SaveCRT2Info(pSiS->RelIO+CROFFSET, ModeNo);
	DisableBridge(BaseAddr);
	UnLockCRT2(BaseAddr);
	SetCRT2ModeRegs(BaseAddr,ModeNo);
	if(VBInfo & DISABLE_CRT2_DISPLAY)  {
		LockCRT2(BaseAddr);
		return;
	}
	GetCRT2Data(ROMAddr,ModeNo);
#if 0
	if (VBInfo & SET_CRT2_TO_HIVISION_TV)
		ModifyDotClock();
#endif

	SetGroup1(pScrn,ModeNo);
	SetGroup2(BaseAddr,ROMAddr);     
	SetGroup3(BaseAddr);
	SetGroup4(BaseAddr,ROMAddr,ModeNo);
#if 0
	SetGroup5(BaseAddr,ROMAddr);
#endif

	LockCRT2(BaseAddr);
	if (SetFlag & TV_SIMU_MODE)  {
		temp = *(CARD16 *)(ROMAddr+ModeIDOffset+1);
		if (!(temp & AFTER_LOCK_CRT2))
			SetLockRegs(BaseAddr, 0x04);
	}
	if (!(VBInfo & SET_IN_SLAVE_MODE))
		EnableCRT2(BaseAddr);
	EnableBridge(BaseAddr);
	if (!GetLockInfo(pSiS->RelIO+CROFFSET, 0x04))
		SetLockRegs(BaseAddr, 0x08);

	return;
}



unsigned short
GetRatePtr(ScrnInfoPtr pScrn, unsigned short ModeNo, CARD8 CRTx)
{
	short		index;
	unsigned char	LCDFRateIndexMax[2] = { 1, 3};
	unsigned short	temp;
	unsigned short	p3d4 = SISPTR(pScrn)->RelIO+CROFFSET;
	unsigned char * ROMAddr = SISPTR(pScrn)->BIOS;

	if (ModeNo <= 0x13)			/* Mode No <= 13h then return */
		return (STANDARD_MODE | CRT2_SUPPORT);

	inSISIDXREG(p3d4,0x33,index);		/* Get 3d4 CRTC33 */
	if (CRTx == SELECT_CRT1)
		index &= 0x0F;			/* Frame rate index */
	else
		index = (index & 0xF0) >> 4;

	if(index != 0)	index--;

	if (SetFlag & PROGRAMMING_CRT2)  {
		if (VBInfo & SET_CRT2_TO_LCD)  {
			if (index > LCDFRateIndexMax[LCDResInfo])
				index = LCDFRateIndexMax[LCDResInfo];
		}
	}
	FRateOffset = *(CARD16 *)(ROMAddr+ModeIDOffset+4);
	do {
		temp=*(CARD16 *)(ROMAddr+FRateOffset);
		if (temp==0xFFFF)
			break;
		if ((temp & MODE_INFO_MASK) < ModeType)
			break;
		FRateOffset += EMODE2_SIZE;
		index--;
		if (index < 0)  {
			if (!(VBInfo & SET_CRT2_TO_RAMDAC) &&
				(VBInfo & SET_IN_SLAVE_MODE))  {
				index = 0;
				if (!(temp & INTERLACE_MODE))
					break;
			}
		}
	} while(index>=0);

	FRateOffset -= EMODE2_SIZE;
	if (SetFlag & PROGRAMMING_CRT2)
		return AdjustCRT2Rate(ROMAddr) | ENHANCED_MODE;
	return (ENHANCED_MODE+CRT2_NO_SUPPORT);
}

unsigned short
AdjustCRT2Rate(CARD8 *ROMAddr)
{
	unsigned short	tempax;
	unsigned short	temp;

	tempax = 0;
	if (VBInfo & SET_CRT2_TO_RAMDAC)
		tempax |= SUPPORT_RAMDAC2;
	if (VBInfo & SET_CRT2_TO_LCD)  {
		tempax |= SUPPORT_LCD;
		temp = *(ROMAddr+ModeIDOffset+9);
		if ((LCDResInfo != PANEL_1280x1024) && (temp >= 9))
			tempax = 0;
	}
	if (VBInfo & (SET_CRT2_TO_AVIDEO|SET_CRT2_TO_SVIDEO|SET_CRT2_TO_SCART))
		tempax |= SUPPORT_TV;
	temp = *(CARD16 *)(ROMAddr+ModeIDOffset+1);
	if (!(VBInfo & SET_PAL_TV) && (temp & NO_SUPPORT_SIMU_TV) &&
	     (VBInfo & SET_IN_SLAVE_MODE) && !(VBInfo & SET_NO_SIMU_ON_LOCK))  {
		return CRT2_NO_SUPPORT;
	}
	do  {
		temp = *(CARD16 *)(ROMAddr+FRateOffset);
		if (temp & tempax)
			return CRT2_SUPPORT;
		temp = *(CARD16 *)(ROMAddr+ModeIDOffset+4);
		if (FRateOffset == temp)
			break;
		FRateOffset -= EMODE2_SIZE;
	} while (1);
	FRateOffset = *(CARD16 *)(ROMAddr+ModeIDOffset+4);
	do {
		FRateOffset += EMODE2_SIZE;
		temp = *(CARD16 *)(ROMAddr+FRateOffset);
		if (temp == 0xFFFF)
			return CRT2_NO_SUPPORT;
		if (temp & tempax)
			return CRT2_SUPPORT;
	} while (1);
}

void
SaveCRT2Info(CARD16 p3d4, CARD16 ModeNo)
{
	outSISIDXREG(p3d4, 0x34, ModeNo);
	setSISIDXREG(p3d4, 0x31, ~SET_IN_SLAVE_MODE,
				(VBInfo & SET_IN_SLAVE_MODE) >> 8);
	andSISIDXREG(p3d4, 0x35, 0xF3);
}

void
DisableBridge(CARD16 BaseAddr)
{
	unsigned short	part2_base = BaseAddr+0x10;

	andSISIDXREG(part2_base, 0, 0xDF);
	orSISIDXREG(BaseAddr+SROFFSET, 1, 0x20);	/* DisplayOff */
	andSISIDXREG(BaseAddr+SROFFSET, 0x32, 0xDF);
	andSISIDXREG(BaseAddr+SROFFSET, 0x1E, 0xDF);
}

void
LongWait(CARD16 BaseAddr)
{
	unsigned short	i;
	unsigned short	p3da = BaseAddr + 0x5A;

	for (i=0; i<0xFFFF; i++) {
		if (!(inSISREG(p3da) & 0x08))	break;
	}
	for (i=0; i<0xFFFF; i++) {
		if (inSISREG(p3da) & 0x08)	break;
	}
}

Bool
WaitVBRetrace(CARD16 BaseAddr)
{
	unsigned short	part1_base = BaseAddr+4;
	unsigned short	i;
	unsigned char	temp;

	inSISIDXREG(part1_base, 0, temp);
	if (!(temp & 0x80))
		return FALSE;

	outSISREG(part1_base, 0x25);
	part1_base++;
	for (i=0; i<0xFFFF; i++)  {
		temp = inSISREG(part1_base);
		if (temp & 0x01)	break;
	}
	for (i=0; i<0xFFFF; i++)  {
		temp = inSISREG(part1_base);
		if (!(temp & 0x01))	break;
	}
	return TRUE;
}

void
VBLongWait(CARD16 p3da)
{
	unsigned int	i, j;

	for (i=0; i<3; i++)  {
		if (i & 1)  {	/* VBWaitMode 2 */
			for (j=0; j<0x640000; j++) {
				if (!(inSISREG(p3da) & 0x08))	break;
			}
		} else {	/* VBWaitMode 1 */
			for (j=0; j<0x640000; j++) {
				if ((inSISREG(p3da) & 0x08))	break;
			}
		}
	}
}

void
UnLockCRT2(CARD16 BaseAddr)
{
	orSISIDXREG(BaseAddr+4, 0x24, 1);
}

void
LockCRT2(CARD16 BaseAddr)
{
	andSISIDXREG(BaseAddr+4, 0x24, 0xFE);
}

void
EnableCRT2(CARD16 BaseAddr)
{
	orSISIDXREG(BaseAddr+SROFFSET, 0x1E, 0x20);
}

void
EnableBridge(CARD16 BaseAddr)
{
	unsigned short	part2_base = BaseAddr+0x10;
	unsigned char	temp, temp1;
	
	temp1 = 0;
	inSISIDXREG(BaseAddr+CROFFSET, 0x31, temp);
	if (temp & (SET_IN_SLAVE_MODE >> 8))  {
		inSISIDXREG(BaseAddr+CROFFSET, 0x30, temp);
		if (!(temp & (SET_CRT2_TO_RAMDAC >> 8)))  {
			temp1 = 0x20;
		}
	}
#if 0
	LongWait(BaseAddr+0x5A);
	VBLongWait(BaseAddr);
#endif
	setSISIDXREG(BaseAddr+SROFFSET, 0x32, ~0x20, temp1);

#if 0
	LongWait(BaseAddr+0x5A);
	VBLongWait(BaseAddr);
#endif
	orSISIDXREG(BaseAddr+SROFFSET, 0x1E, 0x20);

	LongWait(BaseAddr+0x5A);
	LongWait(BaseAddr+0x5A);
	LongWait(BaseAddr+0x5A);
#if 0
	VBLongWait(BaseAddr);
#endif
	setSISIDXREG(part2_base, 0, ~0xE0, 0x20);

	LongWait(BaseAddr+0x5A);
	LongWait(BaseAddr+0x5A);
	LongWait(BaseAddr+0x5A);
#if 0
	VBLongWait(BaseAddr);
#endif
	andSISIDXREG(BaseAddr+SROFFSET, 1, ~0x20);	/* DisplayOn */

	LongWait(BaseAddr+0x5A);
	LongWait(BaseAddr+0x5A);
	LongWait(BaseAddr+0x5A);
#if 0
	VBLongWait(BaseAddr);
#endif
}

void
SetCRT2ModeRegs(CARD16 BaseAddr, CARD16 ModeNo)
{
	CARD16	part1_base = BaseAddr+4;
	CARD16	part4_base = BaseAddr+0x14;
	CARD8	tempah;

	outSISIDXREG(part1_base, 4, 0);
	outSISIDXREG(part1_base, 5, 0);
	outSISIDXREG(part1_base, 6, 0);

	/* set index 0 register */
	tempah = 0x80;
	if (ModeNo > 0x13)
		if (ModeType >= MODE_VGA)
			tempah = (0x10 >> (ModeType-MODE_VGA)) | 0x80;
	if (VBInfo & SET_IN_SLAVE_MODE)
		tempah ^= 0xA0;
	if (VBInfo & DISABLE_CRT2_DISPLAY)
		tempah = 0;
	outSISIDXREG(part1_base, 0, tempah);

	/* set index 1 register */
	tempah = 0x01;
	if (!(VBInfo & SET_IN_SLAVE_MODE))
		tempah |= 0x02;
	if (!(VBInfo & SET_CRT2_TO_RAMDAC))  {
		tempah ^= 0x05;
		if (!(VBInfo & SET_CRT2_TO_LCD))
			tempah ^= 0x01;
	}
	tempah <<= 5;
	if (VBInfo & DISABLE_CRT2_DISPLAY)
		tempah = 0;
	outSISIDXREG(part1_base, 1, tempah);

	tempah >>= 5;
	if ((ModeType == MODE_VGA) && (!(VBInfo & SET_IN_SLAVE_MODE)))
		tempah |= 0x10;
	if (LCDResInfo != PANEL_1024x768)
		tempah |= 0x80;
	if ((VBInfo & SET_CRT2_TO_TV) && (VBInfo & SET_IN_SLAVE_MODE))
		tempah |= 0x20;
	setSISIDXREG(part4_base, 0x0D, ~(0xBF), tempah);

	tempah = 0;
	if (VBInfo & SET_CRT2_TO_TV) {
		if (!(VBInfo & SET_IN_SLAVE_MODE) ||
		    !(SetFlag & TV_SIMU_MODE))  {
			SetFlag |= RPLLDIV2XO;
			tempah |= 0x40;
		}
	}
	if (LCDResInfo != PANEL_1024x768)
		tempah |= 0x80;
	outSISIDXREG(part4_base, 0x0C, tempah);
}

void
GetCRT2Data(CARD8 *ROMAddr, CARD16 ModeNo)
{
	unsigned short	tempax, tempbx;

	RVBHRS = 50;
	NewFlickerMode = 0;
	RY1COE = RY2COE = RY3COE = RY4COE = 0;

	GetResInfo(ROMAddr, ModeNo);
	if (VBInfo & SET_CRT2_TO_RAMDAC)  {
		GetRAMDAC2Data(ROMAddr, ModeNo);
		return;
	}
	GetCRT2Ptr(ROMAddr, ModeNo);
	RVBHCMAX = *CRTC2Ptr | GETBITSTR(*(CRTC2Ptr+4), 7:7, 8:8);
	RVBHCFACT = *(CRTC2Ptr+1);
	VGAHT = *((CARD16 *)(CRTC2Ptr+2)) & 0xFFF;
	VGAVT = (*((CARD16 *)(CRTC2Ptr+3)) >> 4) & 0x7FF;
	if (VBInfo & SET_CRT2_TO_LCD)  {
		HT = *((CARD16 *)(CRTC2Ptr+5)) & 0xFFF;
		VT = (*((CARD16 *)(CRTC2Ptr+6)) >> 4) & 0x7FF;
		tempax = 1024;
		tempbx = 560;
		if (VGAVDE != 350)
			tempbx = 640;
		if (VGAVDE != 400)
			tempbx = 768;
		if (LCDResInfo == PANEL_1280x1024)  {
			tempax = 1280;
			tempbx = 768;
			if (VGAVDE != 360)
				tempbx = 800;
			if (VGAVDE != 375)
				tempbx = 864;
			if (VGAVDE != 405)
				tempbx = 1024;
		}
		HDE = tempax;
		VDE = tempbx;
	} else {	/* CRT2ToTV */
		HDE = *(CARD16 *)(CRTC2Ptr+5) & 0xFFF;
		VDE =(*(CARD16 *)(CRTC2Ptr+6) >> 4) & 0x7FF;
		if (*(CARD16 *)(ROMAddr+ModeIDOffset+1) & HALF_DCLK)
			RVBHRS = *(CARD16 *)(CRTC2Ptr+10);
		else
			RVBHRS = *(CARD16 *)(CRTC2Ptr+8) & 0xFFF;
		NewFlickerMode = *(CRTC2Ptr+9) & 0x80;
		if (VBInfo & SET_CRT2_TO_HIVISION_TV)  {
			if (VGAVDE > 480)
				SetFlag &= (~TV_SIMU_MODE);
			if (SetFlag & TV_SIMU_MODE)  {
				HT = ST_HIVISION_TV_HT;
				VT = ST_HIVISION_TV_VT;
			} else {
				HT = EXT_HIVISION_TV_HT;
				VT = EXT_HIVISION_TV_VT;
			}
		} else {
			RY1COE = *(CRTC2Ptr+12);
			RY2COE = *(CRTC2Ptr+13);
			RY3COE = *(CRTC2Ptr+14);
			RY4COE = *(CRTC2Ptr+15);
			if (VBInfo & SET_PAL_TV)  {
				HT = PAL_HT;
				VT = PAL_VT;
			} else {
				HT = NTSC_HT;
				VT = NTSC_VT;
			}
		} 
	} /* end of LCD/TV */
}

void
GetResInfo(CARD8 *ROMAddr, CARD16 ModeNo)
{
	unsigned char	tempal;
	unsigned short	resX, resY;
	unsigned short	mode_flag;

	if (ModeNo <= 0x13)  {
		tempal = *(ROMAddr+ModeIDOffset+5);
		resX = StResInfo[tempal][0];
		resY = StResInfo[tempal][1];
	} else  {
		tempal = *(ROMAddr+ModeIDOffset+9);
		resX = ModeResInfo[tempal][0];
		resY = ModeResInfo[tempal][1];
	}
	if (ModeNo > 0x13)  {
		mode_flag = *(CARD16 *)(ROMAddr+ModeIDOffset+1);
		if (mode_flag & HALF_DCLK)
			resX *= 2;
		if (mode_flag & DOUBLE_SCAN)
			resY *= 2;
	}
	if (LCDResInfo != PANEL_1024x768)  {
		if (resY == 400)  resY = 405;
		if (resY == 350)  resY = 360;
		if (SetFlag & LCDVESA_TIMING)
			if (resY == 360)  resY = 375;
	}
	HDE = VGAHDE = resX;
	VDE = VGAVDE = resY;
}

void
GetRAMDAC2Data(CARD8 *ROMAddr, CARD16 ModeNo)
{
	unsigned short	tempax,		/* horizontal total */
			tempbx;		/* vertical total */
	unsigned char	cr0, cr6, cr7, sra, srb;

	RVBHCMAX=1;
	RVBHCFACT=1;
	if (ModeNo <= 0x13)  {
		/* unused */
		ErrorF("Current we do not support STD mode for LCD/TV\n");
	} else {
		GetCRT1Ptr(ROMAddr);
		cr0 = *(ROMAddr+CRTC1Offset);
		cr6 = *(ROMAddr+CRTC1Offset+6);
		cr7 = *(ROMAddr+CRTC1Offset+7);
		sra = *(ROMAddr+CRTC1Offset+13);
		srb = *(ROMAddr+CRTC1Offset+14);
		tempax = cr0 | GETBITSTR(srb, 1:0, 9:8);
		tempbx = cr6 | GETBITSTR(cr7, 0:0, 8:8) |
			GETBITSTR(cr7, 5:5, 9:9) | GETBITSTR(sra, 0:0, 10:10);
		if (*(CARD16 *)(ROMAddr+ModeIDOffset+1) & CHAR_8DOT)
			HT = VGAHT = tempax * 8;
		else
			HT = VGAHT = tempax * 9;
		VT = VGAVT = tempbx + 1;
	}
}

Bool
GetLockInfo(CARD16 p3d4, CARD8 info)
{
	unsigned char	temp;

	inSISIDXREG(p3d4, 0x35, temp);	
	return ((temp & info)!=0);
}

void
SetLockRegs(CARD16 BaseAddr, CARD8 info)
{
	if (!(VBInfo & SET_IN_SLAVE_MODE))
		return;
	if (VBInfo & SET_CRT2_TO_RAMDAC)
		return;
	orSISIDXREG(BaseAddr+CROFFSET, 0x35, info);
	LongWait(BaseAddr+0x5a);
	orSISIDXREG(BaseAddr+SROFFSET, 0x32, 0x20);
}

void
GetCRT1Ptr(CARD8 *ROMAddr)
{
	CRTC1Offset = *((CARD16 *)(ROMAddr+0x204)) +
			*(ROMAddr+FRateOffset+2) * 17;
}

void
GetCRT2Ptr(CARD8 *ROMAddr, CARD16 ModeNo)
{
	unsigned char	index;
	if (ModeNo <= 0x13)
		index = *(ROMAddr+ModeIDOffset+4);
	else
		index = *(ROMAddr+FRateOffset+4) & 0x1F;

	if (VBInfo & SET_CRT2_TO_LCD)  {
		if (LCDResInfo == PANEL_1024x768)
			if (SetFlag & LCDVESA_TIMING)
				CRTC2Ptr = ExtLCD1Data[index];
			else
				CRTC2Ptr = StLCD1Data[index];
		else
			if (SetFlag & LCDVESA_TIMING)
				CRTC2Ptr = ExtLCD2Data[index];
			else
				CRTC2Ptr = StLCD2Data[index];
	} else if (VBInfo & SET_CRT2_TO_HIVISION_TV) {
		if (SetFlag & TV_SIMU_MODE)
			CRTC2Ptr = StHiTVData[index];
		else
			CRTC2Ptr = ExtHiTVData[index];
	} else {
		if (VBInfo & SET_PAL_TV)  {
			if (SetFlag & TV_SIMU_MODE)
				CRTC2Ptr = StPALData[index];
			else
				CRTC2Ptr = ExtPALData[index];
		} else  {
			if (SetFlag & TV_SIMU_MODE)
				CRTC2Ptr = StNTSCData[index];
			else
				CRTC2Ptr = ExtNTSCData[index];
		}
	}
}

void
SetGroup1(ScrnInfoPtr pScrn, CARD16 ModeNo)
{
	CARD16	part1_base = SISPTR(pScrn)->RelIO+4;
	CARD8 * ROMAddr = SISPTR(pScrn)->BIOS;
	CARD8 * CRTC1Ptr = ROMAddr+CRTC1Offset;
	CARD16	tempax, tempbx, tempcx, temp;

	ErrorF("Enter SetGroup1()\n");
	SetCRT2Offset(pScrn);
	SetCRT2FIFO(pScrn);
	SetCRT2Sync(ROMAddr, part1_base);
	GetCRT1Ptr(ROMAddr);

	outSISIDXREG(part1_base, 8, GETVAR8(VGAHT-1));
	setSISIDXREG(part1_base, 9, GENMASK(3:0), GETBITSTR(VGAHT-1,11:8,7:4));
	outSISIDXREG(part1_base, 0x0A, GETVAR8(VGAHDE+12));

	tempcx = (VGAHT - VGAHDE) >> 2;
	tempbx = VGAHDE + 12;
	tempbx += tempcx;
	tempcx = tempbx + (tempcx << 1);
	if (VBInfo & SET_CRT2_TO_RAMDAC)  {
		tempbx = *(CRTC1Ptr+4) | GETBITSTR(*(CRTC1Ptr+14), 7:6, 9:8);
		tempbx = (tempbx-1) << 3;
		tempcx = (*(CRTC1Ptr+5) & 0x1F) | ((*(CRTC1Ptr+15) & 4) << 3);
		tempcx = (tempcx-1) << 3;
	}
	outSISIDXREG(part1_base, 0x0B, GETVAR8(tempbx));
	outSISIDXREG(part1_base, 0x0C, GETBITSTR(VGAHDE+12,11:8,7:4) |
					GETBITSTR(tempbx,11:8,3:0));
	outSISIDXREG(part1_base, 0x0D, GETVAR8(tempcx));
	outSISIDXREG(part1_base, 0x0E, GETVAR8(VGAVT-1));
	outSISIDXREG(part1_base, 0x0F, GETVAR8(VGAVDE-1));
	outSISIDXREG(part1_base, 0x12, GETBITSTR(VGAVT-1,10:8, 2:0) |
					GETBITSTR(VGAVDE-1, 10:8, 5:3));
	tempbx = (VGAVDE + VGAVT) >> 1;
	tempcx = ((VGAVT - VGAVDE) >> 4) + tempbx + 1;
	if (VBInfo & SET_CRT2_TO_RAMDAC)  {
		tempbx = *(CRTC1Ptr+8);
		if (*(CRTC1Ptr+7) & 4)
			tempbx |= 0x100;
		if (*(CRTC1Ptr+7) & 0x80)
			tempbx |= 0x200;
		if (*(CRTC1Ptr+13) & 8)
			tempbx |= 0x400;
		tempcx = *(CRTC1Ptr+9);
	}
	outSISIDXREG(part1_base, 0x10, GETVAR8(tempbx));
	outSISIDXREG(part1_base, 0x11, GETBITS(tempcx, 3:0) |
					GETBITSTR(tempbx,10:8, 6:4));
	temp = 0x10;
	if (LCDResInfo != PANEL_1024x768)
		temp = 0x20;
	if (VBInfo & SET_CRT2_TO_TV)  {
		if (VBInfo & SET_CRT2_TO_HIVISION_TV)
			temp = 0x20;
		else
			temp = 8;
	}
	setSISIDXREG(part1_base, 0x13, ~0x3C, temp);

	if (!(VBInfo & SET_IN_SLAVE_MODE))
		return;

	/* skip set in slave/lock mode */

	ErrorF("Leave SetGroup1()\n");
}

void
SetGroup2(CARD16 BaseAddr, CARD8 *ROMAddr)
{
	CARD16	part2_base = BaseAddr+0x10;
	CARD8	temp;
	CARD16	tempax, tempcx, tempbx, *Data16Ptr;

	ErrorF("Enter SetGroup2()\n");
	temp = (VBInfo & SET_CRT2_TO_AVIDEO) << 1 |
		(VBInfo & SET_CRT2_TO_SVIDEO) >> 1 |
		(VBInfo & SET_CRT2_TO_SCART) >> 3 |
		(VBInfo & SET_CRT2_TO_HIVISION_TV) >> 7 ;
	temp ^= 0x0C;
	if (!(VBInfo & (SET_CRT2_TO_HIVISION_TV | SET_PAL_TV)))
		temp |= 0x10;
	outSISIDXREG(part2_base, 0, temp);

	if (VBInfo & (SET_CRT2_TO_HIVISION_TV | SET_PAL_TV))
		SetBlock(part2_base, 0x31, 0x34, ROMAddr+0xF1);
	else
		SetBlock(part2_base, 0x31, 0x34, ROMAddr+0xED);

	if (VBInfo & SET_CRT2_TO_HIVISION_TV)  {
		if (SetFlag & TV_SIMU_MODE)  {
			SetBlock(part2_base, 0x01, 0x2D, HiTVTimingSimu1);
			SetBlock(part2_base, 0x39, 0x45, HiTVTimingSimu2);
			Data16Ptr = HiTVTimingSimu3;
		} else  {
			SetBlock(part2_base, 0x01, 0x2D, HiTVTiming1);
			SetBlock(part2_base, 0x39, 0x45, HiTVTiming2);
			Data16Ptr = HiTVTiming3;
		}
	} else  {
		if (VBInfo & SET_PAL_TV)  {
			SetBlock(part2_base, 0x01, 0x2D, PALTiming1);
			SetBlock(part2_base, 0x39, 0x45, PALTiming2);
			Data16Ptr = PALTiming3;
		} else  {
			SetBlock(part2_base, 0x01, 0x2D, NTSCTiming1);
			SetBlock(part2_base, 0x39, 0x45, NTSCTiming2);
			Data16Ptr = NTSCTiming3;
		}
	}

	orSISIDXREG(part2_base, 0x0A, NewFlickerMode);
	outSISIDXREG(part2_base, 0x35, RY1COE);
	outSISIDXREG(part2_base, 0x36, RY2COE);
	outSISIDXREG(part2_base, 0x37, RY3COE);
	outSISIDXREG(part2_base, 0x38, RY4COE);
	outSISIDXREG(part2_base, 0x1B, GETVAR8(HT-1));
	setSISIDXREG(part2_base, 0x1D, ~0x0F, GETBITS(HT-1,11:8));

	if (VBInfo & SET_CRT2_TO_HIVISION_TV)
		tempcx = (HT >> 1) + 3;
	else
		tempcx = (HT >> 1) + 7;
	setSISIDXREG(part2_base, 0x22, ~0xF0, GETBITSTR(tempcx, 3:0, 7:4));

	tempbx = Data16Ptr[0] + tempcx;
	outSISIDXREG(part2_base, 0x24, GETVAR8(tempbx));
	setSISIDXREG(part2_base, 0x25, ~0xF0, GETBITSTR(tempbx, 11:8, 7:4));

	if (VBInfo & SET_CRT2_TO_HIVISION_TV)
		tempcx = tempbx += 4;
	else
		tempbx += 8;
	setSISIDXREG(part2_base, 0x29, ~0xF0, GETBITSTR(tempbx, 3:0, 7:4));

	tempcx += Data16Ptr[1];
	outSISIDXREG(part2_base, 0x27, GETVAR8(tempcx));
	setSISIDXREG(part2_base, 0x28, ~0xF0, GETBITSTR(tempcx, 11:8, 7:4));

	if (VBInfo & SET_CRT2_TO_HIVISION_TV)
		tempcx += 4;
	else
		tempcx += 8;
	setSISIDXREG(part2_base, 0x2A, ~0xF0, GETBITSTR(tempcx, 3:0, 7:4));
	tempcx = (HT >> 1) - Data16Ptr[2];
	setSISIDXREG(part2_base, 0x2D, ~0xF0, GETBITSTR(tempcx, 3:0, 7:4));
	tempcx -= 11;
	if (!(VBInfo & SET_CRT2_TO_TV))
		tempcx = GetVGAHT2()-1;
	outSISIDXREG(part2_base, 0x2E, GETVAR8(tempcx));
	switch (VGAVDE)  {
	case 360:
	case 375:
		tempbx = 746;
		break;
	case 405:
		tempbx = 853;
		break;
	default:
		tempbx = VDE;
	}
	if (VBInfo & SET_CRT2_TO_TV)
		tempbx >>= 1;
	tempbx -= 2;
	outSISIDXREG(part2_base, 0x2F, GETVAR8(tempbx));

	temp = GETBITS(tempcx, 11:8) | GETBITSTR(tempbx, 9:8, 7:6);
	temp |= 0x10;
	if (!(VBInfo & SET_CRT2_TO_SVIDEO))
		temp |= 0x20;
	outSISIDXREG(part2_base, 0x30, temp);

	tempbx = 0;
	temp = 0;
	if ( !(*(CARD16 *)(ROMAddr+ModeIDOffset+1) & HALF_DCLK) )  {
		tempcx = VGAHDE;
		if (tempcx >= HDE)  {
			temp |= 0x20;
		}
	}
	tempcx = 0x0101;
	if (VBInfo & SET_CRT2_TO_HIVISION_TV)  {
		if (VGAHDE >= 1024)  {
			tempcx = 0x1920;
			if (VGAHDE >= 1280)  {
				tempcx = 0x1420;
				temp &= (~0x20);
			}
		}
	}
	if (!(temp & 0x20))  {
		if (*(CARD16 *)(ROMAddr+ModeIDOffset+1) & HALF_DCLK)
			tempcx = (tempcx & 0xFF00) | GETVAR8(tempcx << 1);
		tempbx = (VGAHDE * (tempcx >> 8) / GETVAR8(tempcx) *
			 8*1024 + (HDE-1)) / HDE;
	}
	outSISIDXREG(part2_base, 0x44, GETVAR8(tempbx));
	setSISIDXREG(part2_base, 0x45, ~0x3F, temp | GETBITS(tempbx, 12:8));

	if (VBInfo & SET_CRT2_TO_TV)
		return;

	outSISIDXREG(part2_base, 0x2C, GETVAR8(HDE-1));
	setSISIDXREG(part2_base, 0x2B, ~0xF0, GETBITSTR(HDE-1, 11:8, 7:4));
	if ((LCDResInfo == PANEL_1280x1024) &&
		(ModeType == MODE_EGA) && (VGAHDE >= 1024)) {
		outSISIDXREG(part2_base, 0x0B, 2);
	} else {
		outSISIDXREG(part2_base, 0x0B, 1);
	}

	outSISIDXREG(part2_base, 0x03, GETVAR8(VDE-1));
	setSISIDXREG(part2_base, 0x0C, ~0x07, GETBITS(VDE-1, 10:8));

	outSISIDXREG(part2_base, 0x19, GETVAR8(VT-1));
	if (LCDInfo & LCDRGB18BIT)
		temp = 0;
	else
		temp = 0x10;
	setSISIDXREG(part2_base, 0x1A, ~0xF0, GETBITSTR(VT-1, 10:8,7:5) | temp);

	tempbx = 768;
	if (LCDResInfo != PANEL_1024x768)
		tempbx = 1024;
	tempax = 1;
	if (tempbx != VDE)
		tempax = (tempbx-VDE) >> 1;
	tempcx = VT - tempax;
	tempbx -= tempax;
	outSISIDXREG(part2_base, 5, GETVAR8(tempcx));
	outSISIDXREG(part2_base, 6, GETVAR8(tempbx));
	outSISIDXREG(part2_base, 2, GETBITS(tempcx, 2:0) |
				    GETBITSTR(tempbx, 4:0, 7:3));
	tempbx = VT;
	tempax = VDE;

	tempcx = (VT-VDE) >> 4;
	tempbx = (tempbx+tempax) >> 1;
	outSISIDXREG(part2_base, 4, GETVAR8(tempbx));
	outSISIDXREG(part2_base, 1, GETBITSTR(tempbx, 11:8, 7:4) |
				GETBITS(tempbx+tempcx+1, 3:0));

	andSISIDXREG(part2_base, 0x09, 0xF0);
	andSISIDXREG(part2_base, 0x0A, 0xF0);

	tempcx = (HT-HDE) >> 2;
	tempbx = HDE+7;
	outSISIDXREG(part2_base, 0x23, GETVAR8(tempbx));
	setSISIDXREG(part2_base, 0x25, ~0x0F, GETBITS(tempbx, 11:8));

	outSISIDXREG(part2_base, 0x1F, 0x07);
	andSISIDXREG(part2_base, 0x20, 0x0F);

	tempbx += tempcx;
	outSISIDXREG(part2_base, 0x1C, GETVAR8(tempbx));
	setSISIDXREG(part2_base, 0x1D, ~0xF0, GETBITSTR(tempbx, 11:8, 7:4));

	tempbx += tempcx;
	outSISIDXREG(part2_base, 0x21, GETVAR8(tempbx));

	andSISIDXREG(part2_base, 0x17, 0xFB);
	andSISIDXREG(part2_base, 0x18, 0xDF);

	ErrorF("Leave SetGroup2()\n");
}

void
SetCRT2Offset(ScrnInfoPtr pScrn)
{
	unsigned short	offset;
	SISPtr		pSiS;
	CARD16		part1_base;

	if (VBInfo & SET_IN_SLAVE_MODE)
		return;

	pSiS = SISPTR(pScrn);
	part1_base = pSiS->RelIO+4;
	offset = pSiS->scrnOffset >> 3;

	outSISIDXREG(part1_base, 7, GETVAR8(offset));
	setSISIDXREG(part1_base, 9,
			~GENMASK(3:0), GETBITSTR(offset, 11:8, 3:0));

	offset = (offset >> 3)+1;			/* SetFIFOStop */
	outSISIDXREG(part1_base, 3, GETVAR8(offset));
}

static	char	timing[8] = {1, 2, 2, 3, 0, 1, 1, 2};
static	short	factor[12][2] = {{81, 4}, {72, 6}, {88, 8}, {120, 12},
				 {55, 4}, {54, 6}, {66, 8}, { 90, 12},
				 {42, 4}, {45, 6}, {55, 8}, { 75, 12}};
void
SetCRT2FIFO(ScrnInfoPtr pScrn)
{
	unsigned int	Low;
	unsigned int	High;
	SISPtr		pSiS = SISPTR(pScrn);
#if 0
	unsigned int	temp, dclk, vclk;
#endif
	unsigned short	part1_base = pSiS->RelIO+4;

	Low = 0x14;
	High = 0x17;
	if (pSiS->Chipset == PCI_CHIP_SIS300)  {
		if (Low > 0x0F)
			High = 0x16;
		else
			High = 0x13;
	}
	setSISIDXREG(part1_base, 2, ~0x1F, Low);
	setSISIDXREG(part1_base, 1, ~0x1F, High);
}

void
SetCRT2Sync(CARD8 * ROMAddr, CARD16 part1_base)
{
	setSISIDXREG(part1_base, 0x19, ~0xC0,
		(*(CARD16 *)(ROMAddr+FRateOffset) >> 8) & 0xC0);
}

void
SetBlock(CARD16 port, CARD8 from, CARD8 to, CARD8 *DataPtr)
{
	CARD8	index;

	for (index=from; index <= to; index++, DataPtr++)  {
		outSISIDXREG(port, index, *DataPtr);
	}
}
unsigned short
GetVGAHT2()
{
	return ((VT-VDE)*RVBHCFACT*HT) / ((VGAVT-VGAVDE)*RVBHCMAX);
}

void
SetGroup3(CARD16 BaseAddr)
{
	ErrorF("Enter SetGroup3()\n");
	if (VBInfo & SET_CRT2_TO_HIVISION_TV)
		if (SetFlag & TV_SIMU_MODE)
			SetBlock(BaseAddr+0x12, 0, 0x3E, HiTVGroup3Simu);
		else
			SetBlock(BaseAddr+0x12, 0, 0x3E, HiTVGroup3Data);
	else
		if (VBInfo & SET_PAL_TV)
			SetBlock(BaseAddr+0x12, 0, 0x3E, PALGroup3Data);
		else
			SetBlock(BaseAddr+0x12, 0, 0x3E, NTSCGroup3Data);
	ErrorF("Leave SetGroup3()\n");
}

void
SetGroup4(CARD16 BaseAddr, CARD8 *ROMAddr, CARD16 ModeNo)
{
	CARD16	part4_base = BaseAddr+0x14;
	CARD8	temp;
	CARD16	tempax, tempbx, tempcx;
	CARD32	temp32;

	ErrorF("Enter SetGroup4()\n");
	outSISIDXREG(part4_base, 0x13, RVBHCFACT);
	outSISIDXREG(part4_base, 0x14, GETVAR8(RVBHCMAX));
	outSISIDXREG(part4_base, 0x16, GETVAR8(VGAHT-1));
	tempcx = VGAVT-1;
	if (!(VBInfo & SET_CRT2_TO_TV))
		tempcx -= 5;
	outSISIDXREG(part4_base, 0x17, GETVAR8(tempcx));
	outSISIDXREG(part4_base, 0x15, GETBITSTR(RVBHCMAX, 7:7, 8:8) |
					GETBITSTR(VGAHT-1, 11:8, 6:3) |
					GETBITS(tempcx, 10:8));

	tempbx = VGAHDE;
	if (*(CARD16 *)(ROMAddr+ModeIDOffset+1) & HALF_DCLK)
		tempbx >>= 1;
	switch (VBInfo & (SET_CRT2_TO_TV | SET_CRT2_TO_LCD)) {
	case SET_CRT2_TO_HIVISION_TV:
		if (tempbx == 1024)
			temp = 0xA0;
		if (tempbx == 1280)
			temp = 0xC0;
		else
			temp = 0;
		break;
	case SET_CRT2_TO_LCD:
		if (tempbx <= 800)
			temp = 0;
		else
			temp = 0x60;
		break;
	default:
		temp = 0x80;
	}
	if (LCDResInfo != PANEL_1280x1024)
		temp |= 0x0A;
	setSISIDXREG(part4_base, 0x0E, ~0xEF, temp);

	if ((VBInfo & SET_CRT2_TO_HIVISION_TV) && !(temp & 0xE0))
		tempbx = VDE >> 1;
	else
		tempbx = VDE;
	outSISIDXREG(part4_base, 0x18, GETVAR8(RVBHRS));
	if (VGAVDE >= tempbx)  {
		tempax = VGAVDE - tempbx;
		temp = 0x40;
	} else {
		tempax = VGAVDE;
		temp = 0;
	}
	temp32 = (tempax * 256*1024 + (tempbx-1)) / tempbx;
	outSISIDXREG(part4_base, 0x1B, GETVAR8(temp32));
	outSISIDXREG(part4_base, 0x1A, GETBITS(temp32, 15:8));
	outSISIDXREG(part4_base, 0x19, GETBITS(RVBHRS, 11:8) |
				GETBITSTR(temp32, 17:16, 5:4) | temp);
	SetCRT2VCLK(BaseAddr, ROMAddr, ModeNo);
	ErrorF("Leave SetGroup4()\n");
}

void
SetCRT2VCLK(CARD16 BaseAddr, CARD8 *ROMAddr, CARD16 ModeNo)
{
	CARD8	*VCLK2Ptr;
	CARD16	part4_base = BaseAddr + 0x14;

	VCLK2Ptr = GetVCLK2Ptr(BaseAddr, ROMAddr, ModeNo);
	outSISIDXREG(part4_base, 0x0A, 1);
	outSISIDXREG(part4_base, 0x0B, *(VCLK2Ptr+1));
	outSISIDXREG(part4_base, 0x0A, *(VCLK2Ptr));
	outSISIDXREG(part4_base, 0x12, 0);
	if (VBInfo & SET_CRT2_TO_RAMDAC)  {
		orSISIDXREG(part4_base, 0x12, 0x28);
	} else {
		orSISIDXREG(part4_base, 0x12, 0x08);
	}
}

CARD8 *
GetVCLK2Ptr(CARD16 BaseAddr, CARD8 *ROMAddr, CARD16 ModeNo)
{
	CARD8	index;
	CARD16	VCLKTableBase;

	if (ModeNo <= 0x13)
		index = *(ROMAddr+ModeIDOffset+4);
	else
		index = *(ROMAddr+FRateOffset+4);

	switch (VBInfo & 0xFC)  {
	case SET_CRT2_TO_HIVISION_TV:
		if (SetFlag & RPLLDIV2XO)
			index = HITVVCLKDIV2;
		else
			index = HITVVCLK;
		if (SetFlag & TV_SIMU_MODE)
			index = HITVSIMUVCLK;
		break;
	case SET_CRT2_TO_LCD:
		if (LCDResInfo == PANEL_1024x768)
			index = VCLK65;
		else
			index = VCLK108_2;
		break;
	case SET_CRT2_TO_RAMDAC:
		if (ModeNo <= 0x13)  {
			index = GETBITS(inSISREG(BaseAddr+0x4C), 3:2);
		} else {
			index = *(ROMAddr+FRateOffset+3);
		}
		break;
	default:
		if (SetFlag & RPLLDIV2XO)
			index = TVVCLKDIV2;
		else
			index = TVVCLK;
	}
	VCLKTableBase = *(CARD16 *)(ROMAddr+0x208);
	return (ROMAddr + VCLKTableBase + index*4);
}

void
SetGroup5(CARD16 BaseAddr, CARD8 *ROMAddr)
{
	ErrorF("Enter Group5()\n");
	if (ModeType != MODE_VGA)
		return;
	if (VBInfo & (SET_IN_SLAVE_MODE | DISABLE_LOAD_CRT2DAC))
		return;

	ErrorF("Group5 begin to write DAC\n");
	EnableCRT2(BaseAddr);
	ErrorF("Group5 end to write DAC\n");
#if 0
	LoadDAC();
#endif
	ErrorF("Leave SetGroup5()\n");
}
