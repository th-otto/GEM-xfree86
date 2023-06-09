/*
 * (c) Copyright 1996 Joseph Moss <joe@XFree86.Org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * JOSEPH MOSS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Joseph Moss shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Joseph Moss.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/SuperProbe/Alliance.c,v 3.4 1999/11/19 13:54:13 hohndel Exp $ */

#include "Probe.h"

static Word Ports[] = {0x000, 0x000, SEQ_IDX, SEQ_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Word))

static int MemProbe_Alliance __STDCARGS((int));

Chip_Descriptor Alliance_Descriptor = {
	"Alliance",
	Probe_Alliance,
	Ports,
	NUMPORTS,
	FALSE,
	FALSE,
	TRUE,
	MemProbe_Alliance,
};

Bool Probe_Alliance(Chipset)
int *Chipset;
{
        Bool result = FALSE;
	Byte idx;
	char chipid[9];
	int i = 0;

	if (!NoPCI)
	{
	    while ((pcrp = pci_devp[i]) != (struct pci_config_reg *)NULL) {
		if (pcrp->_vendor == PCI_VENDOR_ALLIANCE && pcrp->_status_command & 7)
		{
			switch (pcrp->_device)
			{
			case PCI_CHIP_PM6410:
				*Chipset = CHIP_ALSC6410;
				break;
			case PCI_CHIP_PM6422:
				*Chipset = CHIP_ALSC6422;
				break;
			case PCI_CHIP_PMAT24:
				*Chipset = CHIP_ALSCAT24;
				break;
			case PCI_CHIP_PMAT3D:
				*Chipset = CHIP_ALSCAT3D;
				break;
			default:
				Chip_data = pcrp->_device;
				*Chipset = CHIP_ALSC_UNK;
				break;
			}
			PCIProbed = TRUE;
			return(TRUE);
		}
		i++;
	    }
	}

	EnableIOPorts(NUMPORTS, Ports);
	for (idx = 0x11; idx < 0x19; idx++) {
		chipid[idx-0x11] = rdinx(SEQ_IDX, idx);
	}
	DisableIOPorts(NUMPORTS, Ports);

	chipid[7] = '\0';
	if ( !strcmp(chipid, "Pro6410") )
		*Chipset = CHIP_ALSC6410;
	else if ( !strcmp(chipid, "Pro6422") )
		*Chipset = CHIP_ALSC6422;
	else if ( !strcmp(chipid, "Pro6424") )
		*Chipset = CHIP_ALSCAT24;
	else if ( !strcmp(chipid, "ProAT3D") )
		*Chipset = CHIP_ALSCAT3D;
	else
		return(FALSE);

	return(TRUE);
}

static int MemProbe_Alliance(Chipset)
int Chipset;
{
    int mem = 0;

    EnableIOPorts(NUMPORTS, Ports);
    mem = rdinx(SEQ_IDX, 0x20) * 64;
    DisableIOPorts(NUMPORTS, Ports);
    return mem;
}
