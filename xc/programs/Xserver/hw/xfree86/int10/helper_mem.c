/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/helper_mem.c,v 1.2 2000/02/08 13:13:26 eich Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include "xf86.h"
#include "xf86str.h"
#include "compiler.h"
#include "xf86Pci.h"
#define _INT10_PRIVATE
#include "int10Defines.h"
#include "xf86int10.h"
#include "xf86_ansic.h"

#define REG pInt

typedef enum {
    OPT_NOINT10
} INT10Opts;

static OptionInfoRec INT10Options[] = {
    {OPT_NOINT10,       "NoINT10",      OPTV_BOOLEAN,   {0},    FALSE },
    { -1,		NULL,		OPTV_NONE,	{0},	FALSE },
};

	

#ifdef DEBUG
void 
dprint(unsigned long start, unsigned long size)
{
    int i,j;
    char *c = (char *)start;

    for (j = 0; j < (size >> 4); j++) {
	char *d = c;
	ErrorF("\n0x%lx:  ",(unsigned long)c);
	for (i = 0; i<16; i++) 
	    ErrorF("%2.2x ",(unsigned char) (*(c++)));
	c = d;
	for (i = 0; i<16; i++) {
	    ErrorF("%c",((((CARD8)(*c)) > 32) && (((CARD8)(*c)) < 128)) ?
		   (unsigned char) (*(c)): '.');
	    c++;
	}
    }
    ErrorF("\n");
}
#endif


/*
 * here we are really paranoid about faking a "real"
 * BIOS. Most of this information was pulled from
 * dosemu.
 */
void
setup_int_vect(xf86Int10InfoPtr pInt)
{
    const CARD16 cs = 0xF000;
    const CARD16 ip = 0x0;
    int i;
    
    /* let the int vects point to the SYS_BIOS seg */
    for (i=0; i<0x80; i++) {
	MEM_WW(pInt,(i<<2),ip);
	MEM_WW(pInt,((i<<2)+2),cs);
    }
    /* video interrupts default location */
    MEM_WW(pInt,(0x42<<2),0xf065);
    MEM_WW(pInt,(0x10<<2),0xf065);
    MEM_WW(pInt,(0x6D<<2),0xf065);
    /* video param table default location (int 1d) */
    MEM_WW(pInt,(0x1d<<2),0xf0A4);
    /* font tables default location (int 1F) */
    MEM_WW(pInt,(0x1f<<2),0xfa6e);

    /* int 11 default location */
    MEM_WW(pInt,(0x11<<2),0xf84d);
    /* int 12 default location */
    MEM_WW(pInt,(0x12<<2),0xf841);
    /* int 15 default location */
    MEM_WW(pInt,(0x15<<2),0xf859);
    /* int 1A default location */
    MEM_WW(pInt,(0x1a<<2),0xff6e);
    /* int 05 default location */
    MEM_WW(pInt,(0x05<<2),0xff54);
    /* int 08 default location */
    MEM_WW(pInt,(0x08<<2),0xfea5);
    /* int 13 default location (fdd) */
    MEM_WW(pInt,(0x13<<2),0xec59);
    /* int 0E default location */
    MEM_WW(pInt,(0x0e<<2),0xef57);
    /* int 17 default location */
    MEM_WW(pInt,(0x17<<2),0xefd2);
    /* fdd table default location (int 1e) */
    MEM_WW(pInt,(0x1e<<2),0xefc7);
}

int
setup_system_bios(memType base_addr)
{
    char *date = "06/01/99";
    char *eisa_ident = "PCI/ISA";
    CARD16 *base = (CARD16*) base_addr;
    
    /*
     * we trap the "industry standard entry points" to the BIOS
     * and all other locations by filling them with "hlt"
     * TODO: implement hlt-handler for these
     */
    memset((void *)(base),0xf4,0x10000);

    /* set bios date */
    strcpy((((char *)base) + 0xFFF5),date);
    /* set up eisa ident string */
    strcpy((((char *)base) + 0xFFD9),eisa_ident);
    /* write system model id for IBM-AT */
    *(((unsigned char *)base) + 0xFFFE) = 0xfc;

    return 1;
}

void
reset_int_vect(xf86Int10InfoPtr pInt)
{
    MEM_WW(pInt,(0x10<<2),0xf065);    
    MEM_WW(pInt,((0x10<<2)+2),0xf000);
    MEM_WW(pInt,(0x42<<2),0xf065);
    MEM_WW(pInt,((0x42<<2)+2),0xf000);
    MEM_WW(pInt,(0x6D<<2),0xf065);
    MEM_WW(pInt,((0x6D<<2)+2),0xf000);
 }

void
set_return_trap(xf86Int10InfoPtr pInt)
{   
    /*
     * here we also set the exit condition:
     * we return when we encounter 'hlt' (^=0xf4) this
     * will be located at address 0x600 in x86 memory.
     */
    MEM_WB(pInt,0x600,0xf4);
}

Bool
int10skip(ScrnInfoPtr pScrn, int entityIndex)
{
    Bool noint10 = FALSE;
    EntityInfoPtr pEnt = xf86GetEntityInfo(entityIndex);
    
    if (pEnt->device && pEnt->device->options) {
	xf86ProcessOptions(pScrn->scrnIndex, pEnt->device->options,
			   INT10Options);
	xf86GetOptValBool(INT10Options, OPT_NOINT10, &noint10);
    }
    xfree(pEnt);

    return noint10;
}
