/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/lib/Xanti/XAntiint.h,v 1.1 1998/11/15 04:29:57 dawes Exp $ */

#ifndef XANTIINT_H
#define XANTIINT_H

#define NEED_REPLIES

#include "Xlibint.h"
#include "XAnti.h"

#if defined(__STDC__) && !defined(UNIXCPP)
#define XAntiGetReq(name, req) \
        WORD64ALIGN\
	if ((dpy->bufptr + SIZEOF(xAnti##name##Req)) > dpy->bufmax)\
		_XFlush(dpy);\
	req = (xAnti##name##Req *)(dpy->last_req = dpy->bufptr);\
	req->reqType = _XAntiCodes->major_opcode;\
        req->xAntiReqType = XAnti_##name; \
        req->length = (SIZEOF(xAnti##name##Req))>>2;\
	dpy->bufptr += SIZEOF(xAnti##name##Req);\
	dpy->request++

#else  /* non-ANSI C uses empty comment instead of "##" for token concatenation */
#define XAntiGetReq(name, req) \
        WORD64ALIGN\
	if ((dpy->bufptr + SIZEOF(xAnti/**/name/**/Req)) > dpy->bufmax)\
		_XFlush(dpy);\
	req = (xAnti/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
	req->reqType = _XAntiCodes->major_opcode;\
	req->xAntiReqType = XAnti_/**/name;\
	req->length = (SIZEOF(xAnti/**/name/**/Req))>>2;\
	dpy->bufptr += SIZEOF(xAnti/**/name/**/Req);\
	dpy->request++
#endif


#endif /* XANTIINT_H */
