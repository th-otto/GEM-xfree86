/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaBitOrder.c,v 1.5 1998/09/05 06:37:03 dawes Exp $ */

#include "xf86.h"
#include "xf86_ansic.h"
#ifndef RAMDAC_MODULE
#include "xaa.h"
#include "xaalocal.h"
#else
#include "xf86CursorPriv.h"
#endif


#if defined(__GNUC__) && defined(__i386__) && !defined(DLOPEN_HACK)
CARD32
XAAReverseBitOrder(CARD32 data)
{
#if defined(Lynx) || (defined(SYSV) || defined(SVR4)) && !defined(ACK_ASSEMBLER) || defined(__ELF__) || defined(__GNU__)
	__asm__(
		"movl $0,%%ecx\n"
		"movb %%al,%%cl\n"
		"movb byte_reversed(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb byte_reversed(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		"movb %%al,%%cl\n"
		"movb byte_reversed(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb byte_reversed(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		: "=a" (data) : "0" (data)
		: "cx"
		);
#else
	__asm__(
		"movl $0,%%ecx\n"
		"movb %%al,%%cl\n"
		"movb _byte_reversed(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb _byte_reversed(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		"movb %%al,%%cl\n"
		"movb _byte_reversed(%%ecx),%%al\n"
		"movb %%ah,%%cl\n"
		"movb _byte_reversed(%%ecx),%%ah\n"
		"roll $16,%%eax\n"
		: "=a" (data) : "0" (data)
		: "cx"
		);
#endif
	return data;
}
#else
CARD32
XAAReverseBitOrder(CARD32 data)
{
    unsigned char* kludge = (unsigned char*)&data;

    kludge[0] = byte_reversed[kludge[0]];
    kludge[1] = byte_reversed[kludge[1]];
    kludge[2] = byte_reversed[kludge[2]];
    kludge[3] = byte_reversed[kludge[3]];
	
    return data;	
}
#endif
