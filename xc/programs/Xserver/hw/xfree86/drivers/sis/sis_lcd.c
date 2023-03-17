#include "xf86.h"
#include "compiler.h"
#include "xf86PciInfo.h"

#include "sis.h"
#include "sis_regs.h"

#define	LCD_DISABLE		0x00000000
#define	LCD_ENABLE		0x00000001
#define	PANEL_LINK		0x00000002
#define	LCD_SCALED		0x00000004
#define	SIS_VB_301		0x00010000
#define	SIS_VB_LVDS		0x00020000


static Bool	SiS530LCDModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void	SiS530LCDSave(void);
static void	SiS530LCDRestore(void);
static Bool	SiS301LCDModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void	SiS301LCDSave(void);
static void	SiS301LCDRestore(void);
static Bool	SiSLVDSModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void	SiSLVDSSave(void);
static void	SiSLVDSRestore(void);


static Bool
SiS301LCDModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	SISPtr	pSiS = SISPTR(pScrn);

	return TRUE;
}


void SiSLCDPreInit(ScrnInfoPtr pScrn)
{
	SISPtr	pSiS = SISPTR(pScrn);
	int	temp;

	pSiS->LCDFlags = LCD_DISABLE;

	switch(pSiS->Chipset)  {
	case PCI_CHIP_SIS530:
		break;
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
		inSISIDXREG(pSiS->RelIO+SROFFSET, 0x38, temp);
		if (!(temp & 0x20))
			break;
		inSISIDXREG(pSiS->RelIO+CROFFSET, 0x32, temp);
		if (!(temp & 0x08))
			break;
		pSiS->LCDFlags = LCD_ENABLE;
		
		pSiS->ModeInit2 = SiS301LCDModeInit;
		break;
	}
}
