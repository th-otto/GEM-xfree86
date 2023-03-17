/* $XConsortium: lcRM.c,v 1.3 94/01/20 18:07:18 rws Exp $ */
/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 * Bug fixes: Bruno Haible	XFree86 Inc.
 */
/* $XFree86: xc/lib/X11/lcRM.c,v 1.3 2000/02/12 05:43:16 dawes Exp $ */

#include "Xlibint.h"
#include "XlcPubI.h"
#include <stdio.h>

typedef struct _StateRec {
    XLCd lcd;
    XlcConv conv;
} StateRec, *State;

static void
mbinit(state)
    XPointer state;
{
    _XlcResetConverter(((State) state)->conv);
}

/* Transforms one multibyte character, and return a 'char' in the same
   parsing class. Returns the number of consumed bytes in *lenp. */
static char
mbchar(state, str, lenp)
    XPointer state;
    char *str;
    int *lenp;
{
    XlcConv conv = ((State) state)->conv;
    char *from;
    wchar_t *to, wc;
    int cur_max, i, from_left, to_left, ret;

    cur_max = XLC_PUBLIC(((State) state)->lcd, mb_cur_max);
    if (cur_max == 1) {
	*lenp = 1;
	return *str;
    }

    from = str;
    from_left = cur_max;
    /* Avoid overrun error which could occur if from_left > strlen(str). */
    for (i = 0; i < cur_max; i++)
	if (str[i] == '\0') {
	    from_left = i;
	    break;
	}
    *lenp = from_left;

    to = &wc;
    to_left = 1;

    ret = _XlcConvert(conv, (XPointer *) &from, &from_left,
		      (XPointer *) &to, &to_left, NULL, 0);
    *lenp -= from_left;

    if (ret < 0 || to_left > 0) {
	/* Invalid or incomplete multibyte character seen. */
	*lenp = 1;
	return 0x7f;
    }
    /* Return a 'char' equivalent to wc. */
    return (wc >= 0 && wc <= 0x7f ? wc : 0x7f);
}

static void
mbfinish(state)
    XPointer state;
{
}

static char *
lcname(state)
    XPointer state;
{
    return ((State) state)->lcd->core->name;
}

static void
destroy(state)
    XPointer state;
{
    _XlcCloseConverter(((State) state)->conv);
    _XCloseLC(((State) state)->lcd);
    Xfree((char *) state);
}

static XrmMethodsRec rm_methods = {
    mbinit,
    mbchar,
    mbfinish,
    lcname,
    destroy
} ;

XrmMethods
_XrmDefaultInitParseInfo(lcd, rm_state)
    XLCd lcd;
    XPointer *rm_state;
{
    State state;

    state = (State) Xmalloc(sizeof(StateRec));
    if (state == NULL)
	return (XrmMethods) NULL;

    state->lcd = lcd;
    state->conv = _XlcOpenConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar);
    if (state->conv == NULL) {
	Xfree((char *) state);
	return (XrmMethods) NULL;
    }
    
    *rm_state = (XPointer) state;
    
    return &rm_methods;
}
