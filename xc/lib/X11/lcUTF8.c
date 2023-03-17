/* $TOG:  $ */
/******************************************************************

              Copyright 1993 by SunSoft, Inc.
              Copyright 1999-2000 by Bruno Haible

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the names of SunSoft, Inc. and
Bruno Haible not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.  SunSoft, Inc. and Bruno Haible make no representations
about the suitability of this software for any purpose.  It is
provided "as is" without express or implied warranty.

SunSoft Inc. AND Bruno Haible DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL SunSoft, Inc. OR Bruno Haible BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/
/* $XFree86: xc/lib/X11/lcUTF8.c,v 1.2 2000/02/29 03:09:04 dawes Exp $ */

/*
 * This file contains:
 *
 * I. Conversion routines CompoundText/CharSet <--> Unicode/UTF-8.
 *
 *    Used for two purposes:
 *      1. The UTF-8 locales, see below.
 *      2. Unicode aware applications for which the use of 8-bit character
 *         sets is an anachronism.
 *
 * II. An UTF-8 locale loader.
 *     Supports: all locales with codeset UTF-8.
 *     How: Provides converters for UTF-8.
 *     Platforms: all systems.
 */

/*
 * The conversion from UTF-8 to CompoundText is realized in a very
 * conservative way. Recall that CompoundText data is used for inter-client
 * communication purposes. We distinguish three classes of clients:
 * - Clients which accept only those pieces of CompoundText which belong to
 *   the character set understood by the current locale.
 *   (Example: clients which are linked to an older X11 library.)
 * - Clients which accept CompoundText with multiple character sets and parse
 *   it themselves.
 *   (Example: emacs, xemacs.)
 * - Clients which rely entirely on the X{mb,wc}TextPropertyToTextList
 *   functions for the conversion of CompoundText to their current locale's
 *   multi-byte/wide-character format.
 * For best interoperation, the UTF-8 to CompoundText conversion proceeds as
 * follows. For every character, it first tests whether the character is
 * representable in the current locale's original (non-UTF-8) character set.
 * If not, it goes through the list of predefined character sets for
 * CompoundText and tests if the character is representable in that character
 * set. If so, it encodes the character using its code within that character
 * set. If not, it uses an UTF-8-in-CompoundText encapsulation. Since
 * clients of the first and second kind ignore such encapsulated text,
 * this encapsulation is kept to a minimum and terminated as early as possible.
 *
 * In a distant future, when clients of the first and second kind will have
 * disappeared, we will be able to stuff UTF-8 data directly in CompoundText
 * without first going through the list of predefined character sets.
 */

#include "Xlibint.h"
#include "XlcPubI.h"
#include "XlcGeneric.h"

static XlcConv
create_conv(lcd, methods)
    XLCd lcd;
    XlcConvMethods methods;
{
    XlcConv conv;

    conv = (XlcConv) Xmalloc(sizeof(XlcConvRec));
    if (conv == (XlcConv) NULL)
	return (XlcConv) NULL;

    conv->methods = methods;
    conv->state = NULL;

    return conv;
}

static void
close_converter(conv)
    XlcConv conv;
{
    Xfree((char *) conv);
}

/* Replacement character for invalid multibyte sequence or wide character. */
#define BAD_WCHAR ((wchar_t) 0xfffd)
#define BAD_CHAR '?'

/***************************************************************************/
/* Part I: Conversion routines CompoundText/CharSet <--> Unicode/UTF-8.
 *
 * Note that this code works in any locale. We store Unicode values in
 * `wchar_t' variables, but don't pass them to the user.
 *
 * This code has to support all character sets that are used for CompoundText,
 * nothing more, nothing less. See the table in lcCT.c.
 * Since the conversion _to_ CompoundText is likely to need the tables for all
 * character sets at once, we don't use dynamic loading (of tables or shared
 * libraries through iconv()). Use a fixed set of tables instead.
 *
 * We use statically computed tables, not dynamically allocated arrays,
 * because it's more memory efficient: Different processes using the same
 * libX11 shared library share the "text" and read-only "data" sections.
 */

typedef wchar_t original_wchar_t;
#define wchar_t unsigned int
#define conv_t XlcConv

typedef struct _Utf8ConvRec {
    const char *name;
    XrmQuark xrm_name;
#if NeedFunctionPrototypes
    int (* cstowc) (XlcConv, wchar_t *, unsigned char const *, int);
#else
    int (* cstowc) ();
#endif
#if NeedFunctionPrototypes
    int (* wctocs) (XlcConv, unsigned char *, wchar_t, int);
#else
    int (* wctocs) ();
#endif
} Utf8ConvRec, *Utf8Conv;

/*
 * int xxx_cstowc (XlcConv conv, wchar_t *pwc, unsigned char const *s, int n)
 * converts the byte sequence starting at s to a wide character. Up to n bytes
 * are available at s. n is >= 1.
 * Result is number of bytes consumed, or -1 if invalid, or 0 if n too small.
 *
 * int xxx_wctocs (XlcConv conv, unsigned char *r, wchar_t wc, int n)
 * converts the wide character wc to the character set xxx, and stores the
 * result beginning at r. Up to n bytes may be written at r. n is >= 1.
 * Result is number of bytes written, or -1 if invalid, or 0 if n too small.
 */

/* Return code if invalid. (xxx_mbtowc, xxx_wctomb) */
#define RET_ILSEQ      0
/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define RET_TOOFEW(n)  (-1-(n))
/* Return code if output buffer is too small. (xxx_wctomb, xxx_reset) */
#define RET_TOOSMALL   -1

/*
 * The tables below are bijective. It would be possible to extend the
 * xxx_wctocs tables to do some transliteration (e.g. U+201C,U+201D -> 0x22)
 * but *only* with characters not contained in any other table, and *only*
 * when the current locale is not an UTF-8 locale.
 */

#ifdef notused
#include "lcUniConv/ascii.h"
#endif
#include "lcUniConv/iso8859_1.h"
#include "lcUniConv/iso8859_2.h"
#include "lcUniConv/iso8859_3.h"
#include "lcUniConv/iso8859_4.h"
#include "lcUniConv/iso8859_5.h"
#include "lcUniConv/iso8859_6.h"
#include "lcUniConv/iso8859_7.h"
#include "lcUniConv/iso8859_8.h"
#include "lcUniConv/iso8859_9.h"
#include "lcUniConv/iso8859_10.h"
#include "lcUniConv/iso8859_14.h"
#include "lcUniConv/iso8859_15.h"
#include "lcUniConv/iso8859_16.h"
#include "lcUniConv/jisx0201.h"
#include "lcUniConv/tis620.h"
#include "lcUniConv/koi8_r.h"
#include "lcUniConv/koi8_u.h"
#include "lcUniConv/armscii_8.h"
#include "lcUniConv/cp1133.h"
#include "lcUniConv/mulelao.h"
#include "lcUniConv/viscii.h"
#include "lcUniConv/tcvn.h"
#include "lcUniConv/georgian_academy.h"
#include "lcUniConv/georgian_ps.h"

typedef struct {
    unsigned short indx; /* index into big table */
    unsigned short used; /* bitmask of used entries */
} Summary16;

#include "lcUniConv/gb2312.h"
#include "lcUniConv/jisx0208.h"
#include "lcUniConv/jisx0212.h"
#include "lcUniConv/ksc5601.h"
#ifdef notdef
#include "lcUniConv/big5.h"
#endif

static Utf8ConvRec all_charsets[] = {
    { "ISO8859-1", NULLQUARK,
	iso8859_1_mbtowc, iso8859_1_wctomb
    },
    { "ISO8859-2", NULLQUARK,
	iso8859_2_mbtowc, iso8859_2_wctomb
    },
    { "ISO8859-3", NULLQUARK,
	iso8859_3_mbtowc, iso8859_3_wctomb
    },
    { "ISO8859-4", NULLQUARK,
	iso8859_4_mbtowc, iso8859_4_wctomb
    },
    { "ISO8859-5", NULLQUARK,
	iso8859_5_mbtowc, iso8859_5_wctomb
    },
    { "ISO8859-6", NULLQUARK,
	iso8859_6_mbtowc, iso8859_6_wctomb
    },
    { "ISO8859-7", NULLQUARK,
	iso8859_7_mbtowc, iso8859_7_wctomb
    },
    { "ISO8859-8", NULLQUARK,
	iso8859_8_mbtowc, iso8859_8_wctomb
    },
    { "ISO8859-9", NULLQUARK,
	iso8859_9_mbtowc, iso8859_9_wctomb
    },
    { "ISO8859-10", NULLQUARK,
	iso8859_10_mbtowc, iso8859_10_wctomb
    },
    { "ISO8859-14", NULLQUARK,
	iso8859_14_mbtowc, iso8859_14_wctomb
    },
    { "ISO8859-15", NULLQUARK,
	iso8859_15_mbtowc, iso8859_15_wctomb
    },
    { "ISO8859-16", NULLQUARK,
	iso8859_16_mbtowc, iso8859_16_wctomb
    },
    { "JISX0201.1976-0", NULLQUARK,
	jisx0201_mbtowc, jisx0201_wctomb
    },
    { "GB2312.1980-0", NULLQUARK,
	gb2312_mbtowc, gb2312_wctomb
    },
    { "JISX0208.1983-0", NULLQUARK,
	jisx0208_mbtowc, jisx0208_wctomb
    },
    { "JISX0212.1990-0", NULLQUARK,
	jisx0212_mbtowc, jisx0212_wctomb
    },
    { "KSC5601.1987-0", NULLQUARK,
	ksc5601_mbtowc, ksc5601_wctomb
    },
    { "TIS620.2533-1", NULLQUARK,
	tis620_mbtowc, tis620_wctomb
    },
    { "KOI8-R", NULLQUARK,
	koi8_r_mbtowc, koi8_r_wctomb
    },
    { "KOI8-U", NULLQUARK,
	koi8_u_mbtowc, koi8_u_wctomb
    },
    { "ARMSCII-8", NULLQUARK,
	armscii_8_mbtowc, armscii_8_wctomb
    },
    { "IBM-CP1133", NULLQUARK,
	cp1133_mbtowc, cp1133_wctomb
    },
    { "MULELAO-1", NULLQUARK,
	mulelao_mbtowc, mulelao_wctomb
    },
    { "VISCII1.1-1", NULLQUARK,
	viscii_mbtowc, viscii_wctomb
    },
    { "TCVN-5712", NULLQUARK,
	tcvn_mbtowc, tcvn_wctomb
    },
    { "GEORGIAN-ACADEMY", NULLQUARK,
	georgian_academy_mbtowc, georgian_academy_wctomb
    },
    { "GEORGIAN-PS", NULLQUARK,
	georgian_ps_mbtowc, georgian_ps_wctomb
    },
#ifdef notdef
    { "BIG-5", NULLQUARK,
	big5_mbtowc, big5_wctomb
    },
#endif
};

#define all_charsets_count (sizeof(all_charsets)/sizeof(all_charsets[0]))

static void
init_all_charsets()
{
    Utf8Conv convptr;
    int i;

    for (convptr = all_charsets, i = all_charsets_count; i > 0; convptr++, i--)
	convptr->xrm_name = XrmStringToQuark(convptr->name);
}

#define lazy_init_all_charsets()					\
    do {								\
	if (all_charsets[0].xrm_name == NULLQUARK)			\
	    init_all_charsets();					\
    } while (0)

/*
 * UTF-8 itself
 */

static int
utf8_cstowc(pwc, src, n)
    wchar_t *pwc;
    unsigned char const *src;
    int n;
{
    unsigned char c = src[0];

    if (c < 0x80) {
	*pwc = c;
	return 1;
    } else if (c < 0xc2) {
	return -1;
    } else if (c < 0xe0) {
	if (n < 2)
	    return 0;
	if (!((src[1] ^ 0x80) < 0x40))
	    return -1;
	*pwc = ((wchar_t) (c & 0x1f) << 6)
	       | (wchar_t) (src[1] ^ 0x80);
	return 2;
    } else if (c < 0xf0) {
	if (n < 3)
	    return 0;
	if (!((src[1] ^ 0x80) < 0x40 && (src[2] ^ 0x80) < 0x40
              && (c >= 0xe1 || src[1] >= 0xa0)))
	    return -1;
	*pwc = ((wchar_t) (c & 0x0f) << 12)
	       | ((wchar_t) (src[1] ^ 0x80) << 6)
	       | (wchar_t) (src[2] ^ 0x80);
	return 3;
    } else if (c < 0xf8 && sizeof(wchar_t)*8 >= 32) {
	if (n < 4)
	    return 0;
	if (!((src[1] ^ 0x80) < 0x40 && (src[2] ^ 0x80) < 0x40
	      && (src[3] ^ 0x80) < 0x40
              && (c >= 0xf1 || src[1] >= 0x90)))
	    return -1;
	*pwc = ((wchar_t) (c & 0x07) << 18)
	       | ((wchar_t) (src[1] ^ 0x80) << 12)
	       | ((wchar_t) (src[2] ^ 0x80) << 6)
	       | (wchar_t) (src[3] ^ 0x80);
	return 4;
    } else if (c < 0xfc && sizeof(wchar_t)*8 >= 32) {
	if (n < 5)
	    return 0;
	if (!((src[1] ^ 0x80) < 0x40 && (src[2] ^ 0x80) < 0x40
	      && (src[3] ^ 0x80) < 0x40 && (src[4] ^ 0x80) < 0x40
              && (c >= 0xf9 || src[1] >= 0x88)))
	    return -1;
	*pwc = ((wchar_t) (c & 0x03) << 24)
	       | ((wchar_t) (src[1] ^ 0x80) << 18)
	       | ((wchar_t) (src[2] ^ 0x80) << 12)
	       | ((wchar_t) (src[3] ^ 0x80) << 6)
	       | (wchar_t) (src[4] ^ 0x80);
	return 5;
    } else if (c < 0xfe && sizeof(wchar_t)*8 >= 32) {
	if (n < 6)
	    return 0;
	if (!((src[1] ^ 0x80) < 0x40 && (src[2] ^ 0x80) < 0x40
	      && (src[3] ^ 0x80) < 0x40 && (src[4] ^ 0x80) < 0x40
	      && (src[5] ^ 0x80) < 0x40
              && (c >= 0xfd || src[1] >= 0x84)))
	    return -1;
	*pwc = ((wchar_t) (c & 0x01) << 30)
	       | ((wchar_t) (src[1] ^ 0x80) << 24)
	       | ((wchar_t) (src[2] ^ 0x80) << 18)
	       | ((wchar_t) (src[3] ^ 0x80) << 12)
	       | ((wchar_t) (src[4] ^ 0x80) << 6)
	       | (wchar_t) (src[5] ^ 0x80);
	return 6;
    } else
	return -1;
}

static int
utf8_wctocs(r, wc, n)
    unsigned char *r;
    wchar_t wc;
    int n; /* n == 0 is acceptable */
{
    int count;
    if ((unsigned int) wc < 0x80)
	count = 1;
    else if ((unsigned int) wc < 0x800)
	count = 2;
    else if ((unsigned int) wc < 0x10000)
	count = 3;
    else if ((unsigned int) wc < 0x200000)
	count = 4;
    else if ((unsigned int) wc < 0x4000000)
	count = 5;
    else if ((unsigned int) wc <= 0x7fffffff)
	count = 6;
    else
	return -1;
    if (n < count)
	return 0;
    switch (count) { /* note: code falls through cases! */
	case 6: r[5] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x4000000;
	case 5: r[4] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x200000;
	case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
	case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
	case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
	case 1: r[0] = wc;
    }
    return count;
}

/* from XlcNCharSet to XlcNUtf8String */

static int
cstoutf8(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XlcCharSet charset;
    char *name;
    Utf8Conv convptr;
    int i;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args < 1)
	return -1;

    charset = (XlcCharSet) args[0];
    name = charset->encoding_name;
    /* not charset->name because the latter has a ":GL"/":GR" suffix */

    for (convptr = all_charsets, i = all_charsets_count; i > 0; convptr++, i--)
	if (!strcmp(convptr->name, name))
	    break;
    if (i == 0)
	return -1;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	wchar_t wc;
	int consumed;
	int count;

	consumed = convptr->cstowc(conv, &wc, src, srcend-src);
	if (consumed < 0)
	    return -1;
	if (consumed == 0)
	    break;

	count = utf8_wctocs(dst, wc, dstend-dst);
	if (count == 0)
	    break;
	if (count < 0) {
	    count = utf8_wctocs(dst, BAD_WCHAR, dstend-dst);
	    if (count == 0)
		break;
	    unconv_num++;
	}
	src += consumed;
	dst += count;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_cstoutf8 = {
    close_converter,
    cstoutf8,
    NULL
};

static XlcConv
open_cstoutf8(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    lazy_init_all_charsets();
    return create_conv(from_lcd, &methods_cstoutf8);
}

/* from XlcNUtf8String to XlcNCharSet */

static XlcConv
create_tocs_conv(lcd, methods)
    XLCd lcd;
    XlcConvMethods methods;
{
    XlcConv conv;
    CodeSet *codeset_list;
    int codeset_num;
    int charset_num;
    int i, j, k;
    Utf8Conv *preferred;

    conv = (XlcConv) Xmalloc(sizeof(XlcConvRec));
    if (conv == (XlcConv) NULL)
	return (XlcConv) NULL;

    lazy_init_all_charsets();

    codeset_list = XLC_GENERIC(lcd, codeset_list);
    codeset_num = XLC_GENERIC(lcd, codeset_num);

    charset_num = 0;
    for (i = 0; i < codeset_num; i++)
	charset_num += codeset_list[i]->num_charsets;
    if (charset_num > all_charsets_count)
	charset_num = all_charsets_count;
    preferred = (Utf8Conv *) Xmalloc((charset_num + 1) * sizeof(Utf8Conv));
    if (preferred == (Utf8Conv *) NULL) {
	Xfree((char *) conv);
	return (XlcConv) NULL;
    }

    /* Loop through all codesets mentioned in the locale. */
    charset_num = 0;
    for (i = 0; i < codeset_num; i++) {
	XlcCharSet *charsets = codeset_list[i]->charset_list;
	int num_charsets = codeset_list[i]->num_charsets;
	for (j = 0; j < num_charsets; j++) {
	    char *name = charsets[j]->encoding_name;
	    /* If it wasn't already encountered... */
	    for (k = charset_num-1; k >= 0; k--)
		if (!strcmp(preferred[k]->name, name))
		    break;
	    if (k < 0) {
		/* Look it up in all_charsets[]. */
		for (k = 0; k < all_charsets_count; k++)
		    if (!strcmp(all_charsets[k].name, name)) {
			/* Add it to the preferred set. */
			preferred[charset_num++] = &all_charsets[k];
			break;
		    }
	    }
	}
    }
    preferred[charset_num] = (Utf8Conv) NULL;

    conv->methods = methods;
    conv->state = (XPointer) preferred;

    return conv;
}

static void
close_tocs_converter(conv)
    XlcConv conv;
{
    Xfree((char *) conv);
}

/*
 * Converts a Unicode character to an appropriate character set. The NULL
 * terminated array of preferred character sets is passed as first argument.
 * If successful, *charsetp is set to the character set that was used, and
 * *sidep is set to the character set side (XlcGL or XlcGR).
 */
static int
charset_wctocs(preferred, charsetp, sidep, conv, r, wc, n)
    Utf8Conv *preferred;
    Utf8Conv *charsetp;
    XlcSide *sidep;
    XlcConv conv;
    unsigned char *r;
    wchar_t wc;
    int n;
{
    int count;
    Utf8Conv convptr;
    int i;

    while (*preferred != (Utf8Conv) NULL) {
	convptr = *preferred;
	count = convptr->wctocs(conv, r, wc, n);
	if (count == 0)
	    return 0;
	if (count > 0) {
	    *charsetp = convptr;
	    *sidep = (*r < 0x80 ? XlcGL : XlcGR);
	    return count;
	}
    }
    for (convptr = all_charsets, i = all_charsets_count; i > 0; convptr++, i--) {
	count = convptr->wctocs(conv, r, wc, n);
	if (count == 0)
	    return 0;
	if (count > 0) {
	    *charsetp = convptr;
	    *sidep = (*r < 0x80 ? XlcGL : XlcGR);
	    return count;
	}
    }
    return -1;
}

static int
utf8tocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc;
	int consumed;
	int count;

	consumed = utf8_cstowc(&wc, src, srcend-src);
	if (consumed == 0)
	    break;
	if (consumed < 0) {
	    src++;
	    unconv_num++;
	    continue;
	}

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == 0)
	    break;
	if (count < 0) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src += consumed;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src += consumed;
	dst += count;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8tocs = {
    close_tocs_converter,
    utf8tocs,
    NULL
};

static XlcConv
open_utf8tocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_tocs_conv(from_lcd, &methods_utf8tocs);
}

/* from XlcNUtf8String to XlcNChar */

static int
utf8tocs1(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc;
	int consumed;
	int count;

	consumed = utf8_cstowc(&wc, src, srcend-src);
	if (consumed == 0)
	    break;
	if (consumed < 0) {
	    src++;
	    unconv_num++;
	    continue;
	}

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == 0)
	    break;
	if (count < 0) {
	    src += consumed;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src += consumed;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src += consumed;
	dst += count;
	break;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8tocs1 = {
    close_tocs_converter,
    utf8tocs1,
    NULL
};

static XlcConv
open_utf8tocs1(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_tocs_conv(from_lcd, &methods_utf8tocs1);
}

/* from XlcNUtf8String to XlcNString */

static int
utf8tostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	unsigned char c;
	wchar_t wc;
	int consumed;

	consumed = utf8_cstowc(&wc, src, srcend-src);
	if (consumed == 0)
	    break;
	if (dst == dstend)
	    break;
	if (consumed < 0) {
	    consumed = 1;
	    c = BAD_CHAR;
	    unconv_num++;
	} else {
	    if ((wc & ~(wchar_t)0xff) != 0) {
		c = BAD_CHAR;
		unconv_num++;
	    } else
		c = (unsigned char) wc;
	}
	*dst++ = c;
	src += consumed;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8tostr = {
    close_converter,
    utf8tostr,
    NULL
};

static XlcConv
open_utf8tostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_utf8tostr);
}

/* from XlcNString to XlcNUtf8String */

static int
strtoutf8(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;

    while (src < srcend) {
	int count = utf8_wctocs(dst, *src, dstend-dst);
	if (count == 0)
	    break;
	dst += count;
	src++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec methods_strtoutf8 = {
    close_converter,
    strtoutf8,
    NULL
};

static XlcConv
open_strtoutf8(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_strtoutf8);
}

/* Registers UTF-8 converters for a non-UTF-8 locale. */
void
_XlcAddUtf8Converters(lcd)
    XLCd lcd;
{
    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNUtf8String, open_cstoutf8);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNCharSet, open_utf8tocs);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNChar, open_utf8tocs1);
    _XlcSetConverter(lcd, XlcNString, lcd, XlcNUtf8String, open_strtoutf8);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNString, open_utf8tostr);
}

#undef wchar_t
#define wchar_t original_wchar_t

/***************************************************************************/
/* Part II: An UTF-8 locale loader.
 *
 * Here we can assume that "multi-byte" is UTF-8 and that `wchar_t' is Unicode.
 */

/* from XlcNMultiByte to XlcNWideChar */

static int
utf8towcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    unsigned char const *src;
    unsigned char const *srcend;
    wchar_t *dst;
    wchar_t *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (wchar_t *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	int consumed = utf8_cstowc(dst, src, srcend-src);
	if (consumed < 0) {
	    src++;
	    *dst = BAD_WCHAR;
	    unconv_num++;
	} else {
	    src += consumed;
	}
	dst++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_utf8towcs = {
    close_converter,
    utf8towcs,
    NULL
};

static XlcConv
open_utf8towcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_utf8towcs);
}

/* from XlcNWideChar to XlcNMultiByte */

static int
wcstoutf8(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend) {
	int count = utf8_wctocs(dst, *src, dstend-dst);
	if (count == 0)
	    break;
	if (count < 0) {
	    count = utf8_wctocs(dst, BAD_WCHAR, dstend-dst);
	    if (count == 0)
		break;
	    unconv_num++;
	}
	dst += count;
	src++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstoutf8 = {
    close_converter,
    wcstoutf8,
    NULL
};

static XlcConv
open_wcstoutf8(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_wcstoutf8);
}

/* from XlcNString to XlcNWideChar */

static int
our_strtowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    unsigned char const *src;
    unsigned char const *srcend;
    wchar_t *dst;
    wchar_t *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (wchar_t *) *to;
    dstend = dst + *to_left;

    while (src < srcend && dst < dstend)
	*dst++ = (wchar_t) *src++;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec methods_strtowcs = {
    close_converter,
    our_strtowcs,
    NULL
};

static XlcConv
open_strtowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_strtowcs);
}

/* from XlcNWideChar to XlcNString */

static int
our_wcstostr(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	unsigned int wc = *src++;
	if (wc < 0x80)
	    *dst = wc;
	else {
	    *dst = BAD_CHAR;
	    unconv_num++;
	}
	dst++;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstostr = {
    close_converter,
    our_wcstostr,
    NULL
};

static XlcConv
open_wcstostr(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_wcstostr);
}

/* from XlcNCharSet to XlcNWideChar */

static int
cstowcs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    XlcCharSet charset;
    char *name;
    Utf8Conv convptr;
    int i;
    unsigned char const *src;
    unsigned char const *srcend;
    wchar_t *dst;
    wchar_t *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    if (num_args < 1)
	return -1;

    charset = (XlcCharSet) args[0];
    name = charset->name;

    for (convptr = all_charsets, i = all_charsets_count; i > 0; convptr++, i--)
	if (!strcmp(convptr->name, name)) /* FIXME: charset->side */
	    break;
    if (i == 0)
	return -1;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (wchar_t *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	unsigned int wc;
	int consumed;

	consumed = convptr->cstowc(conv, &wc, src, srcend-src);
	if (consumed < 0)
	    return -1;
	if (consumed == 0)
	    break;

	*dst++ = wc;
	src += consumed;
    }

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return unconv_num;
}

static XlcConvMethodsRec methods_cstowcs = {
    close_converter,
    cstowcs,
    NULL
};

static XlcConv
open_cstowcs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    lazy_init_all_charsets();
    return create_conv(from_lcd, &methods_cstowcs);
}

/* from XlcNWideChar to XlcNCharSet */

static int
wcstocs(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc = *src;
	int count;

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == 0)
	    break;
	if (count < 0) {
	    src++;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src++;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src++;
	dst += count;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstocs = {
    close_tocs_converter,
    wcstocs,
    NULL
};

static XlcConv
open_wcstocs(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_tocs_conv(from_lcd, &methods_wcstocs);
}

/* from XlcNWideChar to XlcNChar */

static int
wcstocs1(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    Utf8Conv *preferred_charsets;
    XlcCharSet last_charset = NULL;
    wchar_t const *src;
    wchar_t const *srcend;
    unsigned char *dst;
    unsigned char *dstend;
    int unconv_num;

    if (from == NULL || *from == NULL)
	return 0;

    preferred_charsets = (Utf8Conv *) conv->state;
    src = (wchar_t const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;
    unconv_num = 0;

    while (src < srcend && dst < dstend) {
	Utf8Conv chosen_charset = NULL;
	XlcSide chosen_side = XlcNONE;
	wchar_t wc = *src;
	int count;

	count = charset_wctocs(preferred_charsets, &chosen_charset, &chosen_side, conv, dst, wc, dstend-dst);
	if (count == 0)
	    break;
	if (count < 0) {
	    src++;
	    unconv_num++;
	    continue;
	}

	if (last_charset == NULL) {
	    last_charset =
	        _XlcGetCharSetWithSide(chosen_charset->name, chosen_side);
	    if (last_charset == NULL) {
		src++;
		unconv_num++;
		continue;
	    }
	} else {
	    if (!(last_charset->xrm_encoding_name == chosen_charset->xrm_name
	          && (last_charset->side == XlcGLGR
	              || last_charset->side == chosen_side)))
		break;
	}
	src++;
	dst += count;
	break;
    }

    if (last_charset == NULL)
	return -1;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    if (num_args >= 1)
	*((XlcCharSet *)args[0]) = last_charset;

    return unconv_num;
}

static XlcConvMethodsRec methods_wcstocs1 = {
    close_tocs_converter,
    wcstocs1,
    NULL
};

static XlcConv
open_wcstocs1(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_tocs_conv(from_lcd, &methods_wcstocs1);
}

/* trivial, no conversion */

static int
identity(conv, from, from_left, to, to_left, args, num_args)
    XlcConv conv;
    XPointer *from;
    int *from_left;
    XPointer *to;
    int *to_left;
    XPointer *args;
    int num_args;
{
    unsigned char const *src;
    unsigned char const *srcend;
    unsigned char *dst;
    unsigned char *dstend;

    if (from == NULL || *from == NULL)
	return 0;

    src = (unsigned char const *) *from;
    srcend = src + *from_left;
    dst = (unsigned char *) *to;
    dstend = dst + *to_left;

    while (src < srcend && dst < dstend)
	*dst++ = *src++;

    *from = (XPointer) src;
    *from_left = srcend - src;
    *to = (XPointer) dst;
    *to_left = dstend - dst;

    return 0;
}

static XlcConvMethodsRec methods_identity = {
    close_converter,
    identity,
    NULL
};

static XlcConv
open_identity(from_lcd, from_type, to_lcd, to_type)
    XLCd from_lcd;
    char *from_type;
    XLCd to_lcd;
    char *to_type;
{
    return create_conv(from_lcd, &methods_identity);
}

XLCd
_XlcUtf8Loader(name)
    _Xconst char *name;
{
    XLCd lcd;

    lcd = _XlcCreateLC(name, _XlcGenericMethods);
    if (lcd == (XLCd) NULL)
	return lcd;

    /* The official IANA name for UTF-8 is "UTF-8" in upper case with a dash. */
    if (!XLC_PUBLIC_PART(lcd)->codeset ||
	(_XlcCompareISOLatin1(XLC_PUBLIC_PART(lcd)->codeset, "UTF-8"))) {
	_XlcDestroyLC(lcd);
	return (XLCd) NULL;
    }

    /* Register elementary converters. */

    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar, open_utf8towcs);

    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte, open_wcstoutf8);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNString, open_wcstostr);

    _XlcSetConverter(lcd, XlcNString, lcd, XlcNWideChar, open_strtowcs);

    /* Register converters for XlcNCharSet. This implicitly provides
     * converters from and to XlcNCompoundText. */

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNMultiByte, open_cstoutf8);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNCharSet, open_utf8tocs);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNChar, open_utf8tocs1);

    _XlcSetConverter(lcd, XlcNCharSet, lcd, XlcNWideChar, open_cstowcs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNCharSet, open_wcstocs);
    _XlcSetConverter(lcd, XlcNWideChar, lcd, XlcNChar, open_wcstocs1);

    _XlcSetConverter(lcd, XlcNString, lcd, XlcNMultiByte, open_strtoutf8);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNString, open_utf8tostr);
    _XlcSetConverter(lcd, XlcNUtf8String, lcd, XlcNMultiByte, open_identity);
    _XlcSetConverter(lcd, XlcNMultiByte, lcd, XlcNUtf8String, open_identity);

    _XlcAddUtf8Converters(lcd);

    return lcd;
}
