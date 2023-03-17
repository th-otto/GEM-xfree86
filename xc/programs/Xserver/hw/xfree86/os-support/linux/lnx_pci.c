/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_pci.c,v 3.4 2000/02/08 17:19:22 dawes Exp $ */

#include <stdio.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "xf86Pci.h"

Bool
xf86GetPciSizeFromOS(PCITAG tag, int index, int* bits)
{
    FILE *file;
    char c[0x100];
    char *res;
    int bus, devfn, dev, fn;
    unsigned int size[7];
    unsigned int num;
    int Size;

    if (index > 7)
	return FALSE;
    
    if (!(file = fopen("/proc/bus/pci/devices","r")))
	return FALSE;
    do {
	res = fgets(c,0xff,file);
	if (res) {
#if defined WORD64 || defined LONG64
#define ILF "\t%*16x\t%*16x\t%*16x\t%*16x\t%*16x\t%*16x\t%*16x"
#define LF  "\t%16x\t%16x\t%16x\t%16x\t%16x\t%16x\t%16x"
#else
#define ILF "\t%*08x\t%*08x\t%*08x\t%*08x\t%*08x\t%*08x\t%*08x"
#define LF  "\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x"
#endif
	    num = sscanf(res,"%02x%02x\t%*04x%*04x\t%*x"ILF LF,
			 &bus,&devfn,&size[0],&size[1],&size[2],&size[3],
			 &size[4],&size[5],&size[6]);
	    if (num != 9) {  /* apparantly not 2.3 style */ 
		fclose(file);
		return FALSE;
	    }
	    dev = devfn >> 3;
	    fn = devfn & 0x7;
	    if (tag == pciTag(bus,dev,fn)) {
		*bits = 0;
		if (size[index] != 0) {
		    Size = size[index] - 1;
		    while (Size & 0x01) {
			Size = Size >> 1;
			(*bits)++;
		    }
		}
		fclose(file);
		return TRUE;
	    }
	}
    } while (res);

    fclose(file);
    return FALSE;
}
