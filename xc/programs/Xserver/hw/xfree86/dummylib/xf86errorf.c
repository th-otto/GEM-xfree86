/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86errorf.c,v 1.1 2000/02/13 03:06:40 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "dummylib.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
xf86ErrorF(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(1, format, ap);
    va_end(ap);
}

