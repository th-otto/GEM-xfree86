/*
 * $XConsortium: PsAttVal.c /main/1 1996/11/16 15:24:44 rws $
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
/* $XFree86: xc/programs/Xserver/Xprint/ps/PsAttVal.c,v 1.3 1999/03/02 11:49:25 dawes Exp $ */

#include "Ps.h"
#include "AttrValid.h"

/*
 * define valid values and defaults for Printer pool
 */
static XpOid ValidContentOrientationsOids[] = {
    xpoid_val_content_orientation_portrait,
    xpoid_val_content_orientation_landscape,
    xpoid_val_content_orientation_reverse_portrait,
    xpoid_val_content_orientation_reverse_landscape 
};
static XpOidList ValidContentOrientations = {
    ValidContentOrientationsOids, XpNumber(ValidContentOrientationsOids)
};

static XpOid DefaultContentOrientationsOids[] = {
    xpoid_val_content_orientation_portrait,
    xpoid_val_content_orientation_landscape
};
static XpOidList DefaultContentOrientations = {
    DefaultContentOrientationsOids, XpNumber(DefaultContentOrientationsOids)
};

static XpOid ValidPlexesOids[] = {
    xpoid_val_plex_simplex, xpoid_val_plex_duplex, xpoid_val_plex_tumble
};
static XpOidList ValidPlexes = {
    ValidPlexesOids, XpNumber(ValidPlexesOids)
};

static XpOid DefaultPlexesOids[] = {
    xpoid_val_plex_simplex
};
static XpOidList DefaultPlexes = {
    DefaultPlexesOids, XpNumber(DefaultPlexesOids)
};

static unsigned long ValidPrinterResolutionsCards[] = {
    300,
    600
};
static XpOidCardList ValidPrinterResolutions = {
    ValidPrinterResolutionsCards, XpNumber(ValidPrinterResolutionsCards)
};

static unsigned long DefaultPrinterResolutionsCards[] = {
    300
};
static XpOidCardList DefaultPrinterResolutions = {
    DefaultPrinterResolutionsCards, XpNumber(DefaultPrinterResolutionsCards)
};

static XpOid ValidListfontsModesOids[] = {
    xpoid_val_xp_list_internal_printer_fonts, xpoid_val_xp_list_glyph_fonts
};
static XpOidList ValidListfontsModes = {
    ValidListfontsModesOids, XpNumber(ValidListfontsModesOids)
};

static XpOid DefaultListfontsModesOids[] = {
    xpoid_val_xp_list_glyph_fonts
};
static XpOidList DefaultListfontsModes = {
    DefaultListfontsModesOids, XpNumber(DefaultListfontsModesOids)
};

static XpOid ValidSetupProvisoOids[] = {
    xpoid_val_xp_setup_mandatory, xpoid_val_xp_setup_optional
};
static XpOidList ValidSetupProviso = {


    ValidSetupProvisoOids, XpNumber(ValidSetupProvisoOids)
};

static XpOidDocFmt ValidDocFormatsSupportedFmts[] = {
    { "Postscript", "2", NULL }
};
static XpOidDocFmtList ValidDocFormatsSupported = {
    ValidDocFormatsSupportedFmts, XpNumber(ValidDocFormatsSupportedFmts)
};

static XpOidDocFmt DefaultDocFormatsSupportedFmts[] = {
    { "Postscript", "2", NULL }
};
static XpOidDocFmtList DefaultDocFormatsSupported = {
    DefaultDocFormatsSupportedFmts, XpNumber(DefaultDocFormatsSupportedFmts)
};

static XpOidDocFmt ValidEmbeddedFormatsSupportedFmts[] = {
    { "Postscript", "2", NULL }
};
static XpOidDocFmtList ValidEmbeddedFormatsSupported = {
    ValidEmbeddedFormatsSupportedFmts, XpNumber(ValidEmbeddedFormatsSupportedFmts)
};

static XpOidDocFmt DefaultEmbeddedFormatsSupportedFmts[] = {
    { "Postscript", "2", NULL }
};
static XpOidDocFmtList DefaultEmbeddedFormatsSupported = {
    DefaultEmbeddedFormatsSupportedFmts, XpNumber(DefaultEmbeddedFormatsSupportedFmts)
};

/*
**	So filtered printers that accept other raw formats can be
**	used with this driver.
**
**		Noah Roberts (jik-)
*/
#if 0
static XpOidDocFmt ValidRawFormatsSupportedFmts[] = {
    { "Postscript", "2", NULL }
    
};
static XpOidDocFmtList ValidRawFormatsSupported = {
    ValidRawFormatsSupportedFmts, XpNumber(ValidRawFormatsSupportedFmts)
};
#endif

static XpOidDocFmt DefaultRawFormatsSupportedFmts[] = {
    { "Postscript", "2", NULL }
};
static XpOidDocFmtList DefaultRawFormatsSupported = {
    DefaultRawFormatsSupportedFmts, XpNumber(DefaultRawFormatsSupportedFmts)
};

static XpOid ValidInputTraysOids[] = {
    xpoid_val_input_tray_manual,
    xpoid_val_input_tray_main,
    xpoid_val_input_tray_envelope,
    xpoid_val_input_tray_large_capacity,
    xpoid_val_input_tray_bottom
};
static XpOidList ValidInputTrays = {
    ValidInputTraysOids, XpNumber(ValidInputTraysOids)
};

static XpOid ValidMediumSizesOids[] = {
    xpoid_val_medium_size_iso_a4,
    xpoid_val_medium_size_na_letter,
    xpoid_val_medium_size_na_legal,
    xpoid_val_medium_size_executive,
    xpoid_val_medium_size_iso_designated_long,
    xpoid_val_medium_size_na_number_10_envelope
};
static XpOidList ValidMediumSizes = {
    ValidMediumSizesOids, XpNumber(ValidMediumSizesOids)
};

static XpOidDocFmt DefaultDocumentFormat = {
    "Postscript", "2", NULL
};


/*
 * init struct for XpValidate*Pool
 */
XpValidatePoolsRec PsValidatePoolsRec = {
    &ValidContentOrientations, &DefaultContentOrientations,
    &ValidDocFormatsSupported, &DefaultDocFormatsSupported,
    &ValidInputTrays, &ValidMediumSizes,
    &ValidPlexes, &DefaultPlexes,
    &ValidPrinterResolutions, &DefaultPrinterResolutions,
    &ValidEmbeddedFormatsSupported, &DefaultEmbeddedFormatsSupported,
    &ValidListfontsModes, &DefaultListfontsModes,
    NULL /* Any raw format specified (NR)*/, &DefaultRawFormatsSupported,
    &ValidSetupProviso,
    &DefaultDocumentFormat
};
