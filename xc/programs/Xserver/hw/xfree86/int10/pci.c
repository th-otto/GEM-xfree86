/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/pci.c,v 1.2 2000/02/08 13:13:26 eich Exp $ */

/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86str.h"
#include "xf86_ansic.h"
#define _INT10_PRIVATE
#include "xf86int10.h"

int
mapPciRom(xf86Int10InfoPtr pInt, unsigned char * address)
{
    PCITAG tag;
    unsigned long offset = 0;
    unsigned char *mem, *ptr;
    unsigned char *scratch = NULL;
    int length = 0;

    pciVideoPtr pvp = xf86GetPciInfoForEntity(pInt->entityIndex);
    
    if (pvp == NULL) {
#ifdef DEBUG
 	ErrorF("mapPciRom: no PCI info\n");
#endif
	return 0;
    }
    
    tag = pciTag(pvp->bus,pvp->device,pvp->func);
    
    mem = ptr = xnfcalloc(0x10000, 1);
    if (! xf86ReadPciBIOS(0,tag,offset,mem,0xFFFF) < 0) {
	xfree(mem);
#ifdef DEBUG
 	ErrorF("mapPciRom: cannot read BIOS\n");
#endif
	return 0;
    }

    while ( *ptr == 0x55 && *(ptr+1) == 0xAA) {
	unsigned short data_off = *(ptr+0x18) | (*(ptr+0x19)<< 8);
	unsigned char *data = ptr + data_off;
	unsigned char type;
	
	if (*data!='P' || *(data+1)!='C' || *(data+2)!='I' || *(data+3)!='R')
	    break;
	type = *(data + 0x14);
#ifdef PRINT_PCI
	ErrorF("data segment in BIOS: 0x%x, type: 0x%x ",data_off,type);
#endif
	if (type != 0)	{ /* not PC-AT image: find next one */
	    unsigned int image_length;
	    unsigned char indicator = *(data + 0x15);
	    if (indicator & 0x80) /* last image */
		break;
	    image_length = (*(data + 0x10)
			     | (*(data + 0x11) << 8)) << 9;
#ifdef PRINT_PCI
	    ErrorF("data image length: 0x%x, ind: 0x%x\n",
		     image_length,indicator);
#endif
	     offset = offset + image_length;
	     if (xf86ReadPciBIOS(0,tag,offset,mem,0xFFFF) < 0) {
		 xfree(mem);
#ifdef DEBUG
	ErrorF("mapPciRom: cannot read BIOS\n");
#endif
		 return 0;
	     }
	     continue;
	 }
	 /* OK, we have a PC Image */
	 length = (*(ptr + 2) << 9);
#ifdef PRINT_PCI
	 ErrorF("BIOS length: 0x%x\n",length);
#endif
	 scratch = (unsigned char *)xnfalloc(length);

	 if (xf86ReadPciBIOS(0,tag,offset,scratch,length) < 0) {
	     xfree(mem);
	     xfree(scratch);
#ifdef DEBUG
	ErrorF("mapPciRom: cannot read BIOS\n");
#endif
	     return 0;
	 }
	 break;
     }
    /* unmap/close/disable PCI bios mem */
    xfree(mem);

    if (scratch && length) {
	memcpy(address, scratch, length);
	xfree(scratch);
    }
#ifdef DEBUG
    if (!length)
	ErrorF("mapPciRom: no BIOS found\n");
#endif
#ifdef PRINT_PCI
    if (length)
	dprint(address,0x20);
#endif
    return length;
}

