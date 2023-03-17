/* $XConsortium: lcCharSet.c,v 1.3 95/02/22 22:02:59 kaleb Exp $ */
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
 */
/* $XFree86: xc/lib/X11/lcCharSet.c,v 3.5 2000/02/29 03:09:04 dawes Exp $ */

#include <stdio.h>
#include "Xlibint.h"
#include "XlcPublic.h"

/* EXTERNS */
/* lcCt.c */
extern Bool _XlcParseCharSet(
#if NeedFunctionPrototypes
    XlcCharSet /* charset */
#endif
);

#if NeedVarargsPrototypes
char *
_XlcGetCSValues(XlcCharSet charset, ...)
#else
char *
_XlcGetCSValues(charset, va_alist)
    XlcCharSet charset;
    va_dcl
#endif
{
    va_list var;
    XlcArgList args;
    char *ret;
    int num_args;

    Va_start(var, charset);
    _XlcCountVaList(var, &num_args);
    va_end(var);

    Va_start(var, charset);
    _XlcVaToArgList(var, num_args, &args);
    va_end(var);

    if (args == (XlcArgList) NULL)
	return (char *) NULL;
    
    if (charset->get_values)
	ret = (*charset->get_values)(charset, args, num_args);
    else
	ret = args->name;

    Xfree(args);

    return ret;
}

typedef struct _XlcCharSetListRec {
    XlcCharSet charset;
    struct _XlcCharSetListRec *next;
} XlcCharSetListRec, *XlcCharSetList;

static XlcCharSetList charset_list = NULL;

XlcCharSet
_XlcGetCharSet(name)
    _Xconst char *name;
{
    XlcCharSetList list;
    XrmQuark xrm_name;
    
    xrm_name = XrmStringToQuark(name);

    for (list = charset_list; list; list = list->next) {
	if (xrm_name == list->charset->xrm_name)
	    return (XlcCharSet) list->charset;
    }

    return (XlcCharSet) NULL;
}

XlcCharSet
_XlcGetCharSetWithSide(encoding_name, side)
    _Xconst char *encoding_name;
    XlcSide side;
{
    XlcCharSetList list;
    XrmQuark xrm_encoding_name;
    
    xrm_encoding_name = XrmStringToQuark(encoding_name);

    for (list = charset_list; list; list = list->next) {
	if (list->charset->xrm_encoding_name == xrm_encoding_name
	    && (list->charset->side == XlcGLGR || list->charset->side == side))
	    return (XlcCharSet) list->charset;
    }

    return (XlcCharSet) NULL;
}

Bool
_XlcAddCharSet(charset)
    XlcCharSet charset;
{
    XlcCharSetList list;

    if (_XlcGetCharSet(charset->name))
	return False;

    list = (XlcCharSetList) Xmalloc(sizeof(XlcCharSetListRec));
    if (list == NULL)
	return False;
    
    list->charset = charset;
    list->next = charset_list;
    charset_list = list;

    return True;
}

static XlcResource resources[] = {
    { XlcNName, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, name), XlcGetMask },
    { XlcNEncodingName, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, encoding_name), XlcGetMask },
    { XlcNSide, NULLQUARK, sizeof(XlcSide),
      XOffsetOf(XlcCharSetRec, side), XlcGetMask },
    { XlcNCharSize, NULLQUARK, sizeof(int),
      XOffsetOf(XlcCharSetRec, char_size), XlcGetMask },
    { XlcNSetSize, NULLQUARK, sizeof(int),
      XOffsetOf(XlcCharSetRec, set_size), XlcGetMask },
    { XlcNControlSequence, NULLQUARK, sizeof(char *),
      XOffsetOf(XlcCharSetRec, ct_sequence), XlcGetMask }
};

static char *
get_values(charset, args, num_args)
    register XlcCharSet charset;
    register XlcArgList args;
    register int num_args;
{
    if (resources[0].xrm_name == NULLQUARK)
	_XlcCompileResourceList(resources, XlcNumber(resources));

    return _XlcGetValues((XPointer) charset, resources, XlcNumber(resources),
			 args, num_args, XlcGetMask);
}

XlcCharSet
_XlcCreateDefaultCharSet(name, ct_sequence)
    _Xconst char *name;
    _Xconst char *ct_sequence;
{
    XlcCharSet charset;
    _Xconst char *colon;

    charset = (XlcCharSet) Xmalloc(sizeof(XlcCharSetRec));
    if (charset == NULL)
	return (XlcCharSet) NULL;
    bzero((char *) charset, sizeof(XlcCharSetRec));

    /* Fill in name and xrm_name.  */
    charset->name = (char *) Xmalloc(strlen(name) + strlen(ct_sequence) + 2);
    if (charset->name == NULL) {
	Xfree((char *) charset);
	return (XlcCharSet) NULL;
    }
    strcpy(charset->name, name);
    charset->xrm_name = XrmStringToQuark(charset->name);

    /* Fill in encoding_name and xrm_encoding_name.  */
    if ((colon = strchr(charset->name, ':')) != NULL) {
        unsigned int length = colon - charset->name;
        charset->encoding_name = (char *) Xmalloc(length + 1);
        if (charset->encoding_name == NULL) {
            Xfree((char *) charset->name);
            Xfree((char *) charset);
            return (XlcCharSet) NULL;
        }
        strncpy(charset->encoding_name, charset->name, length);
        charset->encoding_name[length] = '\0';
        charset->xrm_encoding_name = XrmStringToQuark(charset->encoding_name);
    } else {
        charset->encoding_name = charset->name;
        charset->xrm_encoding_name = charset->xrm_name;
    }

    /* Fill in ct_sequence.  */
    charset->ct_sequence = charset->name + strlen(name) + 1;
    strcpy(charset->ct_sequence, ct_sequence);

    charset->get_values = get_values;

    _XlcParseCharSet(charset);

    return (XlcCharSet) charset;
}
