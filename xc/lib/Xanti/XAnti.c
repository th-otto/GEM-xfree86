/*
Copyright (c) 1986  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
*/
/* $XFree86: xc/lib/Xanti/XAnti.c,v 1.2 1999/06/06 08:47:49 dawes Exp $ */

/*
  Based on xc/lib/X11/Text.c and others.  
  Altered for antialiased text by Mark Vojkovich (mvojkovi@ucsd.edu). 
*/

#define NEED_REPLIES
#include <X11/X.h>
#include <X11/Xproto.h>
#include "XAntiint.h"
#include "XAnti.h"
#include "XAntiproto.h"
#include <stdio.h>


static XExtCodes *_XAntiCodes;

#define	PREAMBLE(dpy) {\
  LockDisplay(dpy); \
  if ((!_XAntiCodes) && (!(_XAntiCodes = XInitExtension(dpy, XAntiName)))) {\
	UnlockDisplay(dpy); \
	SyncHandle(); \
	return 0; \
  }}

#define POSTAMBLE(dpy) \
  UnlockDisplay(dpy); \
  SyncHandle()


Bool
XAntiQueryExtension(
    Display *dpy,
    unsigned int *p_version, 
    unsigned int *p_revision
){
   xAntiQueryExtensionReq *req;
   xAntiQueryExtensionReply rep;
 
   PREAMBLE(dpy);

   XAntiGetReq(QueryExtension, req);

   if (_XReply(dpy, (xReply *)&rep, 0, xFalse) == 0) {
      POSTAMBLE(dpy);
      return False;
   }

   POSTAMBLE(dpy);

   *p_version = rep.version;
   *p_revision = rep.revision;
 
   return True;
}


int
XAntiInterpolateColors(
    Display *dpy,
    GC gc,
    Colormap cmap,
    XColor *colors
){
   xAntiInterpolateColorsReq *req;
   xAntiInterpolateColorsReply rep;
   int i;

   if(!colors) return BadValue;
 
   PREAMBLE(dpy);

   FlushGC(dpy, gc);

   XAntiGetReq(InterpolateColors, req);
   req->colormap = cmap;
   req->number = 3;
   req->gc = gc->gid;

   if (_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
   	xColorItem *items;
	int size = 3 * sizeof(xColorItem);

	if((items = (xColorItem*)Xmalloc(size))) {
	    _XRead(dpy, (char*)items, size);
	    for(i = 0; i < 3; i++) {
		colors[i].pixel = items[i].pixel;
		colors[i].red   = items[i].red;
		colors[i].green = items[i].green;
		colors[i].blue  = items[i].blue;
		colors[i].flags = items[i].flags;
	    }
	    Xfree(items);
	} else 
	    _XEatData(dpy, size);
   } else {
	POSTAMBLE(dpy); 
	return BadImplementation;
   }

   POSTAMBLE(dpy); 
   return Success;
}


/* 
   I considered having XAntiSetInterpolationPixels not generate any 
   protocol right off, but wait until it was explicitly flushed.  But
   this complicates things and there is no real benefit.  Unlike 
   the main GC data where you can set a whole bunch of different
   fields without generating any protocol, we are only setting one
   field and the only benefit would be to programmers who can't make
   up their minds and keep changing the interpolation pixels before
   they use them. 
*/ 

int
XAntiSetInterpolationPixels(
   Display *dpy,
   GC gc,
   unsigned long *pixels
){
   xAntiSetInterpolationPixelsReq *req;
   CARD32 toSend[3];

   if(!pixels)
	return BadValue;

   toSend[0] = pixels[0];
   toSend[1] = pixels[1];
   toSend[2] = pixels[2];

   PREAMBLE(dpy);
   XAntiGetReq(SetInterpolationPixels, req);
   req->length += 3;
   req->gc = gc->gid;
   req->number = 3;

   Data(dpy, (char*)toSend, 12);

   POSTAMBLE(dpy);
   return Success;
}


int
XAntiDrawString(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length
){   
    int Datalength = 0;
    xAntiPolyText8Req *req;

    if (length <= 0)
       return 0;

    PREAMBLE(dpy);
    FlushGC(dpy, gc);
    XAntiGetReq (PolyText8, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;

    Datalength += SIZEOF(xTextElt) * ((length + 253) / 254) + length;
    req->length += (Datalength + 3)>>2;  /* convert to number of 32-bit words */

    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     * the buffer 32-bit aligned. 
     */

    if (dpy->bufptr + Datalength > dpy->bufmax)
    	_XFlush (dpy);

    {
	int nbytes;
	int PartialNChars = length;
 	char *CharacterOffset = (char *)string;
        unsigned char *tbuf;

	while(PartialNChars > 254) {
 	    nbytes = 254 + SIZEOF(xTextElt);
	    BufAlloc (unsigned char *, tbuf, nbytes);
            *(unsigned char *)tbuf = 254;
            *(tbuf+1) = 0;
            memcpy ((char *)tbuf+2, CharacterOffset, 254);
	    PartialNChars = PartialNChars - 254;
	    CharacterOffset += 254;
	}
	    
        if (PartialNChars) {
	    nbytes = PartialNChars + SIZEOF(xTextElt);
	    BufAlloc (unsigned char *, tbuf, nbytes); 
            *(unsigned char *)tbuf =  PartialNChars;
            *(tbuf+1) = 0;
	    memcpy ((char *)tbuf+2, CharacterOffset, PartialNChars);
	 }
    }

    /* Pad request out to a 32-bit boundary */

    if (Datalength &= 3) {
	char *pad;
	/* 
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)" 
	 */
	length = 4 - Datalength;
	BufAlloc (char *, pad, length);
	/* 
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt 
	 */
	*pad = 0;
        }

    /* 
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine. 
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);
    POSTAMBLE(dpy);
    return 0;
}

int
XAntiDrawString16(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length
){   
    int Datalength = 0;
    xAntiPolyText16Req *req;

    if (length <= 0)
       return 0;

    PREAMBLE(dpy);
    FlushGC(dpy, gc);
    XAntiGetReq (PolyText16, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;

    Datalength += SIZEOF(xTextElt) * ((length + 253) / 254) + (length << 1);
    req->length += (Datalength + 3)>>2;  /* convert to number of 32-bit words */

    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     * the buffer 32-bit aligned. 
     */

    if (dpy->bufptr + Datalength > dpy->bufmax)
    	_XFlush (dpy);

    {
	int nbytes;
	int PartialNChars = length;
        xTextElt *elt;
 	XChar2b *CharacterOffset = (XChar2b *)string;

	while(PartialNChars > 254) {
 	    nbytes = 254 * 2 + SIZEOF(xTextElt);
	    BufAlloc (xTextElt *, elt, nbytes);
	    elt->delta = 0;
	    elt->len = 254;
#if defined(MUSTCOPY) || defined(MUSTCOPY2B)
	    {
		register int i;
		register unsigned char *cp;
		for (i = 0, cp = ((unsigned char *)elt) + 2; i < 254; i++) {
		    *cp++ = CharacterOffset[i].byte1;
		    *cp++ = CharacterOffset[i].byte2;
		}
	    }
#else
            memcpy ((char *) (elt + 1), (char *)CharacterOffset, 254 * 2);
#endif
	    PartialNChars = PartialNChars - 254;
	    CharacterOffset += 254;
	}
	    
        if (PartialNChars) {
	    nbytes = PartialNChars * 2  + SIZEOF(xTextElt);
	    BufAlloc (xTextElt *, elt, nbytes); 
	    elt->delta = 0;
	    elt->len = PartialNChars;
#if defined(MUSTCOPY) || defined(MUSTCOPY2B)
	    {
		register int i;
		register unsigned char *cp;
		for (i = 0, cp = ((unsigned char *)elt) + 2; i < PartialNChars;
		     i++) {
		    *cp++ = CharacterOffset[i].byte1;
		    *cp++ = CharacterOffset[i].byte2;
		}
	    }
#else
            memcpy((char *)(elt + 1), (char *)CharacterOffset, 
					PartialNChars * 2);
#endif
	 }
    }

    /* Pad request out to a 32-bit boundary */

    if (Datalength &= 3) {
	char *pad;
	/* 
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)" 
	 */
	length = 4 - Datalength;
	BufAlloc (char *, pad, length);
	/* 
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt 
	 */
	*pad = 0;
        }

    /* 
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine. 
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);
    POSTAMBLE(dpy);
    return 0;
}


int
XAntiDrawText(
    Display *dpy,
    Drawable d,
    GC gc,
    int x, 
    int y,
    XTextItem *items,
    int nitems
){   
    int i;
    XTextItem *item;
    int length = 0;
    xAntiPolyText8Req *req;

    PREAMBLE(dpy);
    FlushGC(dpy, gc);
    XAntiGetReq (PolyText8, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;

    item = items;
    for (i=0; i < nitems; i++) {
	if (item->font)
	    length += 5;  /* a 255 byte, plus size of Font id */
        if (item->delta) {
	    if (item->delta > 0)
	      length += SIZEOF(xTextElt) * ((item->delta + 126) / 127);
            else
   	      length += SIZEOF(xTextElt) * ((-item->delta + 127) / 128);
        }
	if (item->nchars > 0) {
	    length += SIZEOF(xTextElt) * ((item->nchars + 253) / 254 - 1);
	    if (!item->delta) length += SIZEOF(xTextElt);
	    length += item->nchars;
     	}
	item++;
    }

    req->length += (length + 3)>>2;  /* convert to number of 32-bit words */


    /* 
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     * the buffer 32-bit aligned. 
     */

    if (dpy->bufptr + length > dpy->bufmax)
    	_XFlush (dpy);

    item = items;
    for (i=0; i< nitems; i++) {

	if (item->font) {
            /* to mark a font shift, write a 255 byte followed by
	       the 4 bytes of font ID, big-end first */
	    register unsigned char *f;
	    BufAlloc (unsigned char *, f, 5);

	    f[0] = 255;
	    f[1] = (item->font & 0xff000000) >> 24;
	    f[2] = (item->font & 0x00ff0000) >> 16;
	    f[3] = (item->font & 0x0000ff00) >> 8;
	    f[4] =  item->font & 0x000000ff;

            /* update GC shadow */
	    gc->values.font = item->font;
	}

	{
	    int nbytes = SIZEOF(xTextElt);
	    int PartialNChars = item->nchars;
	    int PartialDelta = item->delta;
            /* register xTextElt *elt; */
	    int FirstTimeThrough = True;
 	    char *CharacterOffset = item->chars;
            char *tbuf;

	    while((PartialDelta < -128) || (PartialDelta > 127)) {
	    	int nb = SIZEOF(xTextElt);

	    	BufAlloc (char *, tbuf, nb); 
	    	*tbuf = 0;    /*   elt->len  */
	    	if (PartialDelta > 0 )  {
		    *(tbuf+1) = 127;  /* elt->delta  */
		    PartialDelta = PartialDelta - 127;
		} else {
		    /* -128 = 0x8, need to be careful of signed chars... */
		    *((unsigned char *)(tbuf+1)) = 0x80;  /* elt->delta */
		    PartialDelta = PartialDelta + 128;
		}
	    }
	    if (PartialDelta) {
                BufAlloc (char *, tbuf , nbytes); 
	        *tbuf = 0;      /* elt->len */
		*(tbuf+1) = PartialDelta;    /* elt->delta  */
	    }
	    while(PartialNChars > 254) {
		nbytes = 254;
	    	if (FirstTimeThrough) {
		    FirstTimeThrough = False;
		    if (!item->delta) { 
			nbytes += SIZEOF(xTextElt);
	   		BufAlloc (char *, tbuf, nbytes); 
		        *(tbuf+1) = 0;     /* elt->delta */
		    } else {
			char *DummyChar;
		        BufAlloc(char *, DummyChar, nbytes);
		    }
		} else {
 		    nbytes += SIZEOF(xTextElt);
	   	    BufAlloc (char *, tbuf, nbytes);
		    *(tbuf+1) = 0;   /* elt->delta */
		}
		/* watch out for signs on chars */
		*(unsigned char *)tbuf = 254;  /* elt->len */
                memcpy (tbuf+2 , CharacterOffset, 254);
		PartialNChars = PartialNChars - 254;
		CharacterOffset += 254;

	    }
	    if (PartialNChars) {
		nbytes = PartialNChars;
	    	if (FirstTimeThrough) {
		    FirstTimeThrough = False;
		    if (!item->delta) { 
			nbytes += SIZEOF(xTextElt);
	   		BufAlloc (char *, tbuf, nbytes); 
			*(tbuf+1) = 0;   /*  elt->delta  */
		    }  else {
			char *DummyChar;
		        BufAlloc(char *, DummyChar, nbytes);
		    }
		} else {
 		    nbytes += SIZEOF(xTextElt);
	   	    BufAlloc (char *, tbuf, nbytes); 
		    *(tbuf+1) = 0;   /* elt->delta  */
		}
	    	*tbuf = PartialNChars;   /*  elt->len  */
                memcpy (tbuf+2 , CharacterOffset, PartialNChars);
	    }
	}
    item++;
    }

    /* Pad request out to a 32-bit boundary */

    if (length &= 3) {
	char *pad;
	/* 
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)" 
	 */
	length = 4 - length;
	BufAlloc (char *, pad, length);
	/* 
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt 
	 */
	*pad = 0;
        }

    /* 
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine. 
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);
    POSTAMBLE(dpy);
    return 0;
}


int
XAntiDrawText16(
    Display *dpy,
    Drawable d,
    GC gc,
    int x, 
    int y,
    XTextItem16 *items,
    int nitems
){   
    int i;
    XTextItem16 *item;
    int length = 0;
    xAntiPolyText16Req *req;

    PREAMBLE(dpy);
    FlushGC(dpy, gc);
    XAntiGetReq (PolyText16, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->x = x;
    req->y = y;

    item = items;
    for (i=0; i < nitems; i++) {
	if (item->font)
	    length += 5;  /* a 255 byte, plus size of Font id */
        if (item->delta) {
	    if (item->delta > 0)
	      length += SIZEOF(xTextElt) * ((item->delta + 126) / 127);
            else
   	      length += SIZEOF(xTextElt) * ((-item->delta + 127) / 128);
        }
	if (item->nchars > 0) {
	    length += SIZEOF(xTextElt) * ((item->nchars + 253) / 254 - 1);
	    if (!item->delta) length += SIZEOF(xTextElt);
	    length += item->nchars << 1;
     	}
	item++;
    }

    req->length += (length + 3)>>2;  /* convert to number of 32-bit words */


    /*
     * If the entire request does not fit into the remaining space in the
     * buffer, flush the buffer first.   If the request does fit into the
     * empty buffer, then we won't have to flush it at the end to keep
     *  the buffer 32-bit aligned. 
     */

    if (dpy->bufptr + length > dpy->bufmax)
    	_XFlush (dpy);

    item = items;
    for (i=0; i< nitems; i++) {

	if (item->font) {
            /* to mark a font shift, write a 255 byte followed by
	       the 4 bytes of font ID, big-end first */
	    register unsigned char *f;
	    BufAlloc (unsigned char *, f, 5);


	    f[0] = 255;
	    f[1] = (item->font & 0xff000000) >> 24;
	    f[2] = (item->font & 0x00ff0000) >> 16;
	    f[3] = (item->font & 0x0000ff00) >> 8;
	    f[4] =  item->font & 0x000000ff;

            /* update GC shadow */
	    gc->values.font = item->font;
	}

	{
	    int nbytes = SIZEOF(xTextElt);
	    int PartialNChars = item->nchars;
	    int PartialDelta = item->delta;
            xTextElt *elt;
	    int FirstTimeThrough = True;
 	    XChar2b *CharacterOffset = item->chars;

	    while((PartialDelta < -128) || (PartialDelta > 127)) {
	    	int nb = SIZEOF(xTextElt);

	    	BufAlloc (xTextElt *, elt, nb); 
	    	elt->len = 0;
	    	if (PartialDelta > 0 ) {
		    elt->delta = 127;
		    PartialDelta = PartialDelta - 127;
		} else {
		    elt->delta = -128;
		    PartialDelta = PartialDelta + 128;
		}
	    }
	    if (PartialDelta) {
                BufAlloc (xTextElt *, elt, nbytes); 
	        elt->len = 0;
		elt->delta = PartialDelta;
	    }
	    while(PartialNChars > 254) {
		nbytes = 254 * 2;
	    	if (FirstTimeThrough) {
		    FirstTimeThrough = False;
		    if (!item->delta) { 
			nbytes += SIZEOF(xTextElt);
	   		BufAlloc (xTextElt *, elt, nbytes); 
		        elt->delta = 0;
		    } else {
			char *DummyChar;
		        BufAlloc(char *, DummyChar, nbytes);
		    }
		} else {
 		    nbytes += SIZEOF(xTextElt);
	   	    BufAlloc (xTextElt *, elt, nbytes);
		    elt->delta = 0;
		}
	    	elt->len = 254;

#if defined(MUSTCOPY) || defined(MUSTCOPY2B)
		{
		    register int i;
		    register unsigned char *cp;
		    for (i = 0, cp = ((unsigned char *)elt) + 2; i < 254; i++) {
			*cp++ = CharacterOffset[i].byte1;
			*cp++ = CharacterOffset[i].byte2;
		    }
		}
#else
		memcpy ((char *) (elt + 1), (char *)CharacterOffset, 254 * 2);
#endif
		PartialNChars = PartialNChars - 254;
		CharacterOffset += 254;

	    }
	    if (PartialNChars) {
		nbytes = PartialNChars * 2;
	    	if (FirstTimeThrough) {
		    FirstTimeThrough = False;
		    if (!item->delta) { 
			nbytes += SIZEOF(xTextElt);
	   		BufAlloc (xTextElt *, elt, nbytes); 
			elt->delta = 0;
		    } else {
			char *DummyChar;
		        BufAlloc(char *, DummyChar, nbytes);
		    }
		} else {
 		    nbytes += SIZEOF(xTextElt);
	   	    BufAlloc (xTextElt *, elt, nbytes); 
		    elt->delta = 0;
		}
	    	elt->len = PartialNChars;

#if defined(MUSTCOPY) || defined(MUSTCOPY2B)
		{
		    register int i;
		    register unsigned char *cp;
		    for (i = 0, cp = ((unsigned char *)elt) + 2; 
			 i < PartialNChars;
			 i++) {
			*cp++ = CharacterOffset[i].byte1;
			*cp++ = CharacterOffset[i].byte2;
		    }
		}
#else
		memcpy ((char *) (elt + 1), (char *)CharacterOffset,
						PartialNChars * 2);
#endif
	    }
	}
    item++;
    }

    /* Pad request out to a 32-bit boundary */

    if (length &= 3) {
	char *pad;
	/*
	 * BufAlloc is a macro that uses its last argument more than
	 * once, otherwise I'd write "BufAlloc (char *, pad, 4-length)" 
	 */
	length = 4 - length;
	BufAlloc (char *, pad, length);
	/* 
	 * if there are 3 bytes of padding, the first byte MUST be 0
	 * so the pad bytes aren't mistaken for a final xTextElt 
	 */
	*pad = 0;
        }

    /* 
     * If the buffer pointer is not now pointing to a 32-bit boundary,
     * we must flush the buffer so that it does point to a 32-bit boundary
     * at the end of this routine. 
     */

    if ((dpy->bufptr - dpy->buffer) & 3)
       _XFlush (dpy);

    POSTAMBLE(dpy);
    return 1;
}

int
XAntiDrawImageString(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst char *string,
    int length
){   
    xAntiImageText8Req *req;
    char *CharacterOffset = (char *)string;
    int FirstTimeThrough = True;
    int lastX = 0;

    PREAMBLE(dpy);
    FlushGC(dpy, gc);

    while (length > 0) {
	int Unit;

	if (length > 255) Unit = 255;
	else Unit = length;

   	if (FirstTimeThrough) {
	    FirstTimeThrough = False;
        } else {
	    char buf[512];
	    char *ptr, *str;
	    xQueryTextExtentsReq *qreq;
	    xQueryTextExtentsReply rep;
	    int i;

	    GetReq(QueryTextExtents, qreq);
	    qreq->fid = gc->gid;
	    qreq->length += (510 + 3)>>2;
	    qreq->oddLength = 1;
	    str = CharacterOffset - 255;
	    for (ptr = buf, i = 255; --i >= 0; ) {
		*ptr++ = 0;
		*ptr++ = *str++;
	    }
	    Data (dpy, buf, 510);
	    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue))
		break;

	    x = lastX + cvtINT32toInt (rep.overallWidth);
	}

        XAntiGetReq (ImageText8, req);
        req->length += (Unit + 3) >> 2;
        req->nChars = Unit;
        req->drawable = d;
        req->gc = gc->gid;
        req->y = y;

	lastX = req->x = x;
        Data (dpy, CharacterOffset, (long)Unit);
        CharacterOffset += Unit;
	length -= Unit;
    }
    POSTAMBLE(dpy);
    return 0;
}


int
XAntiDrawImageString16(
    Display *dpy,
    Drawable d,
    GC gc,
    int x,
    int y,
    _Xconst XChar2b *string,
    int length
){   
    xAntiImageText16Req *req;
    XChar2b *CharacterOffset = (XChar2b *)string;
    int FirstTimeThrough = True;
    int lastX = 0;

    PREAMBLE(dpy);
    FlushGC(dpy, gc);

    while (length > 0)  {
	int Unit, Datalength;

	if (length > 255) Unit = 255;
	else Unit = length;

   	if (FirstTimeThrough) {
	    FirstTimeThrough = False;
        } else {
	    char buf[512];
	    xQueryTextExtentsReq *qreq;
	    xQueryTextExtentsReply rep;
	    unsigned char *ptr;
	    XChar2b *str;
	    int i;

	    GetReq(QueryTextExtents, qreq);
	    qreq->fid = gc->gid;
	    qreq->length += (510 + 3)>>2;
	    qreq->oddLength = 1;
	    str = CharacterOffset - 255;
	    for (ptr = (unsigned char *)buf, i = 255; --i >= 0; str++) {
		*ptr++ = str->byte1;
		*ptr++ = str->byte2;
	    }
	    Data (dpy, buf, 510);
	    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue))
		break;

	    x = lastX + cvtINT32toInt (rep.overallWidth);
	}

        XAntiGetReq (ImageText16, req);
        req->length += ((Unit << 1) + 3) >> 2;
        req->nChars = Unit;
        req->drawable = d;
        req->gc = gc->gid;
        req->y = y;

	lastX = req->x = x;
	Datalength = Unit << 1;
        Data (dpy, (char *)CharacterOffset, (long)Datalength);
        CharacterOffset += Unit;
	length -= Unit;
    }
    POSTAMBLE(dpy);
    return 0;
}

