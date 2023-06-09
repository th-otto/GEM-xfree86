/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/stub.c,v 1.2 2000/02/12 03:39:57 dawes Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include "xf86.h"
#include "xf86str.h"
#include "xf86_OSproc.h"
#define _INT10_PRIVATE
#include "xf86int10.h"

xf86Int10InfoPtr
xf86InitInt10(int entityIndex)
{
    return NULL;
}

void
MapCurrentInt10(xf86Int10InfoPtr pInt)
{
    return;
}

void
xf86FreeInt10(xf86Int10InfoPtr pInt)
{
    return;
}

void *
xf86Int10AllocPages(xf86Int10InfoPtr pInt,int num, int *off)
{
    *off = 0;
    return NULL;
}

void
xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase, int num)
{
    return;
}

Bool
xf86Int10ExecSetup(xf86Int10InfoPtr pInt)
{
    return FALSE;
}

void
xf86ExecX86int10(xf86Int10InfoPtr pInt)
{
    return;
}

pointer
xf86int10Addr(xf86Int10InfoPtr pInt, CARD32 addr)
{
    return 0;
}
