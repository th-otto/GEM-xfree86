/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_ev56.c,v 3.5 2000/02/17 13:45:49 dawes Exp $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

int readDense8(pointer Base, register unsigned long Offset);
int readDense16(pointer Base, register unsigned long Offset);
int readDense32(pointer Base, register unsigned long Offset);
void
writeDenseNB8(int Value, pointer Base, register unsigned long Offset);
void
writeDenseNB16(int Value, pointer Base, register unsigned long Offset);
void
writeDenseNB32(int Value, pointer Base, register unsigned long Offset);
void
writeDense8(int Value, pointer Base, register unsigned long Offset);
void
writeDense16(int Value, pointer Base, register unsigned long Offset);
void
writeDense32(int Value, pointer Base, register unsigned long Offset);

int
readDense8(pointer Base, register unsigned long Offset)
{
    return *(volatile CARD8*) ((unsigned long)Base+(Offset));
}

int
readDense16(pointer Base, register unsigned long Offset)
{
    return *(volatile CARD16*) ((unsigned long)Base+(Offset));
}

int
readDense32(pointer Base, register unsigned long Offset)
{
    return *(volatile CARD32*)((unsigned long)Base+(Offset));
}

void
writeDenseNB8(int Value, pointer Base, register unsigned long Offset)
{
    *(volatile CARD8*)((unsigned long)Base+(Offset)) = Value;
}

void
writeDenseNB16(int Value, pointer Base, register unsigned long Offset)
{
    *(volatile CARD16*)((unsigned long)Base + (Offset)) = Value;
}

void
writeDenseNB32(int Value, pointer Base, register unsigned long Offset)
{
    *(volatile CARD32*)((unsigned long)Base+(Offset)) = Value;
}

void
writeDense8(int Value, pointer Base, register unsigned long Offset)
{
    *(volatile CARD8 *)((unsigned long)Base+(Offset)) = Value;
    mem_barrier();
}

void
writeDense16(int Value, pointer Base, register unsigned long Offset)
{
    *(volatile CARD16 *)((unsigned long)Base+(Offset)) = Value;
    mem_barrier();
}

void
writeDense32(int Value, pointer Base, register unsigned long Offset)
{
    *(volatile CARD32 *)((unsigned long)Base+(Offset)) = Value;
    mem_barrier();
}
