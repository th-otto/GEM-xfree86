/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/sparcPci.c,v 1.1 1999/03/28 15:32:57 dawes Exp $ */
/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <stdio.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "Pci.h"

#include <asm/unistd.h>
#ifndef __NR_pciconfig_read
#define __NR_pciconfig_read  148
#define __NR_pciconfig_write 149
#endif

/*
 * UltraSPARC platform specific PCI access functions
 */
CARD32 sparcPciCfgRead(PCITAG tag, int off);
void sparcPciCfgWrite(PCITAG, int off, CARD32 val);
void sparcPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits);

pciBusInfo_t sparcPci0 = {
/* configMech  */	  PCI_CFG_MECH_OTHER,
/* numDevices  */	  32,
/* secondary   */	  FALSE,
/* primary_bus */	  0,
/* ppc_io_base */	  0,
/* ppc_io_size */	  0,		  
/* funcs       */	  {
	                    sparcPciCfgRead,
			    sparcPciCfgWrite,
			    sparcPciCfgSetBits,
			    pciAddrNOOP,
			    pciAddrNOOP
		          },
/* pciBusPriv  */	  NULL
};

void  
sparcPciInit()
{
  pciNumBuses    = 1;
  pciBusInfo[0]  = &sparcPci0;
  pciFindFirstFP = pciGenFindFirst;
  pciFindNextFP  = pciGenFindNext;
}


#if defined(linux)
/*
 * These funtions will work for Linux, but other OS's
 * are likely have a different mechanism for getting at
 * PCI configuration space
 */
CARD32
sparcPciCfgRead(PCITAG tag, int off)
{
	int bus, dfn, len;
	CARD32 val = 0xffffffff;

	bus = PCI_BUS_FROM_TAG(tag);
	dfn = PCI_DFN_FROM_TAG(tag);
	
	syscall(__NR_pciconfig_read, bus, dfn, off, 4, &val);
	return(val);
}

void
sparcPciCfgWrite(PCITAG tag, int off, CARD32 val)
{
	int bus, dfn, len;

	bus = PCI_BUS_FROM_TAG(tag);
	dfn = PCI_DFN_FROM_TAG(tag);
	
	syscall(__NR_pciconfig_write, bus, dfn, off, 4, &val);
}

void
sparcPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits)
{
    int bus, dfn, len;
    CARD32 val = 0xffffffff;

    bus = PCI_BUS_FROM_TAG(tag);
    dfn = PCI_DFN_FROM_TAG(tag);
	
    syscall(__NR_pciconfig_read, bus, dfn, off, 4, &val);
    val = (val & ~mask) | (bits & mask);
    syscall(__NR_pciconfig_write, bus, dfn, off, 4, &val);
}

int sparcUseHWMulDiv(void);

#if defined(__GNUC__) && defined(__GLIBC__)
#define HWCAP_SPARC_MULDIV	8
extern unsigned long int _dl_hwcap;
#endif

int
sparcUseHWMulDiv(void)
{
    FILE *f;
    char buffer[1024];
    char *p;
#if defined(__GNUC__) && defined(__GLIBC__)
    unsigned long *hwcap;
    __asm(".weak _dl_hwcap");
    
    hwcap = &_dl_hwcap;
    __asm("" : "=r" (hwcap) : "0" (hwcap));
    if (hwcap) {
        if (*hwcap & HWCAP_SPARC_MULDIV)
    	    return 1;
    	else
    	    return 0;
    }
#endif
    f = fopen("/proc/cpuinfo","r");
    if (!f) return 0;
    while (fgets(buffer, 1024, f) != NULL) {
        if (!strncmp (buffer, "type", 4)) {
            p = strstr (buffer, "sun4");
            if (p && (p[4] == 'u' || p[4] == 'd' || p[4] == 'm')) {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}
#endif /* Linux */
