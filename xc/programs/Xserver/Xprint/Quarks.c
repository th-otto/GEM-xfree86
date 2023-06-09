/* $XFree86: xc/programs/Xserver/Xprint/Quarks.c,v 1.3 1999/12/16 02:26:25 robin Exp $ */

/*
 * $XConsortium: Quarks.c /main/1 1996/09/28 16:58:44 rws $
 */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

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
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/

#include "Xlibint.h"
#include <X11/Xresource.h>

/* Not cost effective, at least for vanilla MIT clients */
/* #define PERMQ */

typedef unsigned long Signature;
typedef unsigned long Entry;
#ifdef PERMQ
typedef unsigned char Bits;
#endif

static XrmQuark nextQuark = 1;	/* next available quark number */
static unsigned long quarkMask = 0;
static Entry zero = 0;
static Entry *quarkTable = &zero; /* crock */
static unsigned long quarkRehash;
static XrmString **stringTable = NULL;
#ifdef PERMQ
static Bits **permTable = NULL;
#endif
static XrmQuark nextUniq = -1;	/* next quark from XrmUniqueQuark */

#define QUANTUMSHIFT	8
#define QUANTUMMASK	((1 << QUANTUMSHIFT) - 1)
#define CHUNKPER	8
#define CHUNKMASK	((CHUNKPER << QUANTUMSHIFT) - 1)

#define LARGEQUARK	((Entry)0x80000000L)
#define QUARKSHIFT	18
#define QUARKMASK	((LARGEQUARK - 1) >> QUARKSHIFT)
#define XSIGMASK	((1L << QUARKSHIFT) - 1)

#define STRQUANTSIZE	(sizeof(XrmString) * (QUANTUMMASK + 1))
#ifdef PERMQ
#define QUANTSIZE	(STRQUANTSIZE + \
			 (sizeof(Bits) * ((QUANTUMMASK + 1) >> 3))
#else
#define QUANTSIZE	STRQUANTSIZE
#endif

#define HASH(sig) ((sig) & quarkMask)
#define REHASHVAL(sig) ((((sig) % quarkRehash) + 2) | 1)
#define REHASH(idx,rehash) ((idx + rehash) & quarkMask)
#define NAME(q) stringTable[(q) >> QUANTUMSHIFT][(q) & QUANTUMMASK]
#ifdef PERMQ
#define BYTEREF(q) permTable[(q) >> QUANTUMSHIFT][((q) & QUANTUMMASK) >> 3]
#define ISPERM(q) (BYTEREF(q) & (1 << ((q) & 7)))
#define SETPERM(q) BYTEREF(q) |= (1 << ((q) & 7))
#define CLEARPERM(q) BYTEREF(q) &= ~(1 << ((q) & 7))
#endif

/* Permanent memory allocation */

#define WALIGN sizeof(unsigned long)
#define DALIGN sizeof(double)

#define NEVERFREETABLESIZE ((8192-12) & ~(DALIGN-1))
static char *neverFreeTable = NULL;
static unsigned  neverFreeTableSize = 0;

static char *permalloc(register unsigned int length)
{
    char *ret;

    if (neverFreeTableSize < length) {
	if (length >= NEVERFREETABLESIZE)
	    return Xmalloc(length);
	if (! (ret = Xmalloc(NEVERFREETABLESIZE)))
	    return (char *) NULL;
	neverFreeTableSize = NEVERFREETABLESIZE;
	neverFreeTable = ret;
    }
    ret = neverFreeTable;
    neverFreeTable += length;
    neverFreeTableSize -= length;
    return(ret);
}

char *Xpermalloc(unsigned int length)
{
    int i;

    if (neverFreeTableSize && length < NEVERFREETABLESIZE) {
#ifndef WORD64
	if ((sizeof(struct {char a; double b;}) !=
	     (sizeof(struct {char a; unsigned long b;}) -
	      sizeof(unsigned long) + sizeof(double))) &&
	    !(length & (DALIGN-1)) &&
	    (i = (NEVERFREETABLESIZE - neverFreeTableSize) & (DALIGN-1))) {
	    neverFreeTableSize -= DALIGN - i;
	    neverFreeTable += DALIGN - i;
	} else
#endif
	    if ((i = (NEVERFREETABLESIZE - neverFreeTableSize) & (WALIGN-1)) != 0) {
		neverFreeTableSize -= WALIGN - i;
		neverFreeTable += WALIGN - i;
	    }
    }
    return permalloc(length);
}

static Bool
ExpandQuarkTable(void)
{
    unsigned long oldmask, newmask;
    register char c, *s;
    register Entry *oldentries, *entries;
    register Entry entry;
    register unsigned long oldidx, newidx, rehash;
    Signature sig;
    XrmQuark q;

    oldentries = quarkTable;
    if ((oldmask = quarkMask) != 0)
	newmask = (oldmask << 1) + 1;
    else {
	if (!stringTable) {
	    stringTable = (XrmString **)Xmalloc(sizeof(XrmString *) *
						CHUNKPER);
	    if (!stringTable)
		return False;
	    stringTable[0] = (XrmString *)NULL;
	}
#ifdef PERMQ
	if (!permTable)
	    permTable = (Bits **)Xmalloc(sizeof(Bits *) * CHUNKPER);
	if (!permTable)
	    return False;
#endif
	stringTable[0] = (XrmString *)Xpermalloc(QUANTSIZE);
	if (!stringTable[0])
	    return False;
#ifdef PERMQ
	permTable[0] = (Bits *)((char *)stringTable[0] + STRQUANTSIZE);
#endif
	newmask = 0x1ff;
    }
    entries = (Entry *)Xmalloc(sizeof(Entry) * (newmask + 1));
    if (!entries)
	return False;
    bzero((char *)entries, sizeof(Entry) * (newmask + 1));
    quarkTable = entries;
    quarkMask = newmask;
    quarkRehash = quarkMask - 2;
    for (oldidx = 0; oldidx <= oldmask; oldidx++) {
	if ((entry = oldentries[oldidx]) != 0) {
	    if (entry & LARGEQUARK)
		q = entry & (LARGEQUARK-1);
	    else
		q = (entry >> QUARKSHIFT) & QUARKMASK;
	    for (sig = 0, s = NAME(q); (c = *s++) != 0; )
		sig = (sig << 1) + c;
	    newidx = HASH(sig);
	    if (entries[newidx]) {
		rehash = REHASHVAL(sig);
		do {
		    newidx = REHASH(newidx, rehash);
		} while (entries[newidx]);
	    }
	    entries[newidx] = entry;
	}
    }
    if (oldmask)
	Xfree((char *)oldentries);
    return True;
}

XrmQuark _XrmInternalStringToQuark(
    register _Xconst char *name, register int len, register Signature sig,
    Bool permstring)
{
    register XrmQuark q;
    register Entry entry;
    register int idx, rehash;
    register int i;
    register char *s1, *s2;
    char *new;

    rehash = 0;
    idx = HASH(sig);
    while ((entry = quarkTable[idx]) != 0) {
	if (entry & LARGEQUARK)
	    q = entry & (LARGEQUARK-1);
	else {
	    if ((entry - sig) & XSIGMASK)
		goto nomatch;
	    q = (entry >> QUARKSHIFT) & QUARKMASK;
	}
	for (i = len, s1 = (char *)name, s2 = NAME(q); --i >= 0; ) {
	    if (*s1++ != *s2++)
		goto nomatch;
	}
	if (*s2) {
nomatch:    if (!rehash)
		rehash = REHASHVAL(sig);
	    idx = REHASH(idx, rehash);
	    continue;
	}
#ifdef PERMQ
	if (permstring && !ISPERM(q)) {
	    Xfree(NAME(q));
	    NAME(q) = (char *)name;
	    SETPERM(q);
	}
#endif
	return q;
    }
    if (nextUniq == nextQuark)
	return NULLQUARK;
    if ((unsigned long)(nextQuark + (nextQuark >> 2)) > quarkMask) {
	if (!ExpandQuarkTable())
	    return NULLQUARK;
	return _XrmInternalStringToQuark(name, len, sig, permstring);
    }
    q = nextQuark;
    if (!(q & QUANTUMMASK)) {
	if (!(q & CHUNKMASK)) {
	    if (!(new = Xrealloc((char *)stringTable,
				 sizeof(XrmString *) *
				 ((q >> QUANTUMSHIFT) + CHUNKPER))))
		return NULLQUARK;
	    stringTable = (XrmString **)new;
#ifdef PERMQ
	    if (!(new = Xrealloc((char *)permTable,
				 sizeof(Bits *) *
				 ((q >> QUANTUMSHIFT) + CHUNKPER))))
		return NULLQUARK;
	    permTable = (Bits **)new;
#endif
	}
	new = Xpermalloc(QUANTSIZE);
	if (!new)
	    return NULLQUARK;
	stringTable[q >> QUANTUMSHIFT] = (XrmString *)new;
#ifdef PERMQ
	permTable[q >> QUANTUMSHIFT] = (Bits *)(new + STRQUANTSIZE);
#endif
    }
    if (!permstring) {
	s2 = (char *)name;
#ifdef PERMQ
	name = Xmalloc(len+1);
#else
	name = permalloc(len+1);
#endif
	if (!name)
	    return NULLQUARK;
	for (i = len, s1 = (char *)name; --i >= 0; )
	    *s1++ = *s2++;
	*s1++ = '\0';
#ifdef PERMQ
	CLEARPERM(q);
    }
    else {
	SETPERM(q);
#endif
    }
    NAME(q) = (char *)name;
    if ((unsigned long)q <= QUARKMASK)
	entry = (q << QUARKSHIFT) | (sig & XSIGMASK);
    else
	entry = q | LARGEQUARK;
    quarkTable[idx] = entry;
    nextQuark++;
    return q;
}

XrmQuark XrmStringToQuark(_Xconst char *name)
{
    register char c, *tname;
    register Signature sig = 0;

    if (!name)
	return (NULLQUARK);
    
    for (tname = (char *)name; (c = *tname++) != 0; )
	sig = (sig << 1) + c;

    return _XrmInternalStringToQuark(name, tname-(char *)name-1, sig, False);
}

XrmQuark XrmPermStringToQuark(_Xconst char *name)
{
    register char c, *tname;
    register Signature sig = 0;

    if (!name)
	return (NULLQUARK);

    for (tname = (char *)name; (c = *tname++) != 0; )
	sig = (sig << 1) + c;

    return _XrmInternalStringToQuark(name, tname-(char *)name-1, sig, True);
}

XrmQuark XrmUniqueQuark(void)
{
    if (nextUniq == nextQuark)
	return NULLQUARK;
    return nextUniq--;
}

XrmString XrmQuarkToString(register XrmQuark quark)
{
    if (quark <= 0 || quark >= nextQuark)
    	return NULLSTRING;
#ifdef PERMQ
    /* We have to mark the quark as permanent, since the caller might hold
     * onto the string pointer forver.
     */
    SETPERM(quark);
#endif
    return NAME(quark);
}
