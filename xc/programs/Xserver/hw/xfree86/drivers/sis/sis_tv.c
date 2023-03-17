#include "xf86.h"
#include "compiler.h"
#include "xf86PciInfo.h"

#include "sis.h"
#include "sis_regs.h"

#define	TV_DISABLE		0
#define	SVIDEO			0x00000001
#define	COMPOSITE		0x00000002
#define	SCART			0x00000004
#define	HIVISION		0x00000008

#define	NTSC			0x00000001
#define	PAL			0x00000002


static Bool	SiS530TVModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void	SiS530TVSave(void);
static void	SiS530TVRestore(void);
static Bool	SiS301TVModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void	SiS301TVSave(void);
static void	SiS301TVRestore(void);


static Bool
SiS530TVModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	SISPtr	pSiS = SISPTR(pScrn);

	return TRUE;
}


void SiSTVPreInit(ScrnInfoPtr pScrn)
{
	SISPtr	pSiS = SISPTR(pScrn);
	int	temp;

	switch(pSiS->Chipset)  {
	case PCI_CHIP_SIS530:
		inSISIDXREG(pSiS->RelIO+SROFFSET, 0x36, temp);
		break;
	case PCI_CHIP_SIS300:
	case PCI_CHIP_SIS630:
	case PCI_CHIP_SIS540:
	
		break;
	default:
		pSiS->TVFlags = TV_DISABLE;
	}
}
