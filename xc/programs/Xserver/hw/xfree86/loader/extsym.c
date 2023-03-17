/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/extsym.c,v 1.3 2000/01/06 20:10:04 mvojkovi Exp $ */

/*
 *
 * Copyright 1999 by The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of The XFree86 Project, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. The XFree86 Project, Inc. makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * THE XFREE86 PROJECT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE XFREE86 PROJECT, INC. BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "resource.h"
#include "sym.h"
#include "misc.h"
#ifdef PANORAMIX
#include "panoramiX.h"
#endif

extern int ShmCompletionCode;
extern int BadShmSegCode;
extern RESTYPE ShmSegType, ShmPixType;

#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
extern int PanoramiXNumScreens;
extern PanoramiXData *panoramiXdataPtr;
extern unsigned long XRT_WINDOW;
extern unsigned long XRT_PIXMAP;
#endif

LOOKUP extLookupTab[] = {

 SYMVAR(ShmCompletionCode)
 SYMVAR(BadShmSegCode)
 SYMVAR(ShmSegType)

#ifdef PANORAMIX
 SYMVAR(noPanoramiXExtension)
 SYMVAR(PanoramiXNumScreens)
 SYMVAR(panoramiXdataPtr)
 SYMVAR(XRT_WINDOW)
 SYMVAR(XRT_PIXMAP)
#endif

 { 0, 0 },

};
