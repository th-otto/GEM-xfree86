/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86opt.c,v 1.1 2000/02/13 03:06:42 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
xf86ProcessOptions(int i, pointer p, OptionInfoPtr o)
{
}

Bool
xf86GetOptValBool(OptionInfoPtr o, int i, Bool *b)
{
    return FALSE;
}

