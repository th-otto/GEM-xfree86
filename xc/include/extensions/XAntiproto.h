/*
   Copyright (C) 1998 by The XFree86 Project Inc.

*/
/* $XFree86: xc/include/extensions/XAntiproto.h,v 1.1 1998/11/15 04:29:56 dawes Exp $ */


#ifndef _XANTIPROTO_H
#define _XANTIPROTO_H

#include <X11/X.h>
#include <X11/Xproto.h>

#define XAntiName "XAnti"
#define XAntiVersion 1
#define XAntiRevision 0

#define XAnti_QueryExtension		0
#define XAnti_SetInterpolationPixels	1
#define XAnti_InterpolateColors		2
#define XAnti_PolyText8			3
#define XAnti_PolyText16		4
#define XAnti_ImageText8		5
#define XAnti_ImageText16		6
#define XAnti_LastRequest		7


#define XAntiNumRequest			XAnti_LastRequest

/* Requests */

typedef struct {
  CARD8 reqType;
  CARD8 xAntiReqType;
  CARD16 length B16;
} xAntiQueryExtensionReq;
#define sz_xAntiQueryExtensionReq 4

typedef struct {
  CARD8 reqType;
  CARD8 xAntiReqType;
  CARD16 length B16;
  GContext gc B32;
  CARD32 number B32;
} xAntiSetInterpolationPixelsReq;
#define sz_xAntiSetInterpolationPixelsReq 12

typedef struct {
  CARD8 reqType;
  CARD8 xAntiReqType;
  CARD16 length B16;
  GContext gc B32;
  Colormap colormap B32;
  CARD32 number;
} xAntiInterpolateColorsReq;
#define sz_xAntiInterpolateColorsReq 16


typedef struct {
    CARD8 reqType;
    CARD8 xAntiReqType;
    CARD16 length B16;
    Drawable drawable B32;
    GContext gc B32;
    INT16 x B16, y B16;		/* items (xTextElt) start after struct */
} xAntiPolyTextReq;    
#define sz_xAntiPolyTextReq 16

typedef xAntiPolyTextReq xAntiPolyText8Req;
typedef xAntiPolyTextReq xAntiPolyText16Req;
#define sz_xAntiPolyText8Req sz_xAntiPolyTextReq
#define sz_xAntiPolyText16Req sz_xAntiPolyTextReq

typedef struct {
    CARD8 reqType;
    CARD8 xAntiReqType;
    CARD16 length B16;
    BYTE nChars;
    CARD8 pad1;
    CARD16 pad2;
    Drawable drawable B32;
    GContext gc B32;
    INT16 x B16, y B16;
} xAntiImageTextReq;    
#define sz_xAntiImageTextReq 20

typedef xAntiImageTextReq xAntiImageText8Req;
typedef xAntiImageTextReq xAntiImageText16Req;
#define sz_xAntiImageText8Req sz_xAntiImageTextReq 
#define sz_xAntiImageText16Req sz_xAntiImageTextReq 

/* Replies */

typedef struct {
  BYTE type;   /* X_Reply */
  CARD8 pad1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD16 version B16;
  CARD16 revision B16;
  CARD32 pad2 B32;
  CARD32 pad3 B32;
  CARD32 pad4 B32;
  CARD32 pad5 B32;
  CARD32 pad6 B32;
} xAntiQueryExtensionReply;
#define sz_xAntiQueryExtensionReply 32


typedef struct {
  BYTE type;   /* X_Reply */
  CARD8 pad1;
  CARD16 sequenceNumber B16;
  CARD32 length B32;
  CARD32 pad2 B32;
  CARD32 pad3 B32;
  CARD32 pad4 B32;
  CARD32 pad5 B32;
  CARD32 pad6 B32;
  CARD32 pad7 B32;
} xAntiInterpolateColorsReply;
#define sz_xAntiInterpolateColorsReply 32


#endif /* _XANTIPROTO_H */
