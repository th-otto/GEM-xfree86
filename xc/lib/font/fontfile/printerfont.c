/* $TOG: printerfont.c /main/4 1998/05/07 15:16:56 kaleb $ */

/*

Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/font/fontfile/printerfont.c,v 1.3 1999/08/21 13:48:04 dawes Exp $ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */
/* $NCDId: @(#)fontfile.c,v 1.6 1991/07/02 17:00:46 lemke Exp $ */

#include    "fntfilst.h"

/*
 * Map FPE functions to renderer functions
 */

#define PRINTERPATHPREFIX  "PRINTER:"

/* STUB
int XpClientIsPrintClient(client,fpe)
pointer		client;
FontPathElementPtr	fpe;
{ return 1; }
 */

static int
PrinterFontNameCheck (char *name)
{
    if (strncmp(name,PRINTERPATHPREFIX,strlen(PRINTERPATHPREFIX)) != 0)
	return 0;
    name += strlen(PRINTERPATHPREFIX);
#ifndef NCD
    return *name == '/';
#else
    return ((strcmp(name, "built-ins") == 0) || (*name == '/'));
#endif
}

static int
PrinterFontInitFPE (FontPathElementPtr fpe)
{
    int			status;
    FontDirectoryPtr	dir;
    char *		name;

    name = fpe->name + strlen(PRINTERPATHPREFIX);
    status = FontFileReadDirectory (name, &dir);
    if (status == Successful)
    {
	if (dir->nonScalable.used > 0)
	    if (!FontFileRegisterBitmapSource (fpe))
	    {
		FontFileFreeFPE (fpe);
		return AllocError;
	    }
	fpe->private = (pointer) dir;
    }
    return status;
}

/* Here we must check the client to see if it has a context attached to
 * it that allows us to access the printer fonts
 */

static int
PrinterFontOpenFont (pointer client, FontPathElementPtr fpe, Mask flags, 
		     char *name, int namelen, 
		     fsBitmapFormat format, fsBitmapFormatMask fmask,
		     XID id, FontPtr *pFont, char **aliasName, 
		     FontPtr non_cachable_font)
{
    if (XpClientIsPrintClient(client,fpe))
	return (FontFileOpenFont  (client, fpe, flags, name, namelen, format, 
		fmask, id, pFont, aliasName, non_cachable_font));
    return BadFontName;
}

static int
PrinterFontListFonts (pointer client, FontPathElementPtr fpe, char *pat, 
		      int len, int max, FontNamesPtr names)
{
    if (XpClientIsPrintClient(client,fpe))
	return FontFileListFonts (client, fpe, pat, len, max, names);
    return BadFontName;
}

static int
PrinterFontStartListFontsWithInfo(pointer client, FontPathElementPtr fpe, 
				  char *pat, int len, int max, 
				  pointer *privatep)
{
    if (XpClientIsPrintClient(client,fpe))
	return FontFileStartListFontsWithInfo(client, fpe, pat, len, 
				max, privatep);
    return BadFontName;
}

static int
PrinterFontListNextFontWithInfo(pointer client, FontPathElementPtr fpe, 
				char **namep, int *namelenp, 
				FontInfoPtr *pFontInfo,
				int *numFonts, pointer private)
{
    if (XpClientIsPrintClient(client,fpe))
	return FontFileListNextFontWithInfo(client, fpe, namep, namelenp, 
				pFontInfo, numFonts, private);
    return BadFontName;
}

static int
PrinterFontStartListFontsAndAliases(pointer client, FontPathElementPtr fpe, 
				    char *pat, int len, int max, 
				    pointer *privatep)
{
    if (XpClientIsPrintClient(client,fpe))
	return FontFileStartListFontsAndAliases(client, fpe, pat, len, 
				max, privatep);
    return BadFontName;
}

static int
PrinterFontListNextFontOrAlias(pointer client, FontPathElementPtr fpe, 
			       char **namep, int *namelenp, 
			       char **resolvedp, int *resolvedlenp, 
			       pointer private)
{
    if (XpClientIsPrintClient(client,fpe))
	return FontFileListNextFontOrAlias(client, fpe, namep, namelenp, 
				resolvedp, resolvedlenp, private);
    return BadFontName;
}

typedef int (*IntFunc) (void);
static int  printer_font_type;

void
PrinterFontRegisterFpeFunctions (void)
{
    /* what is the use of printer font type? */
    printer_font_type = RegisterFPEFunctions(PrinterFontNameCheck,
					  PrinterFontInitFPE,
					  FontFileFreeFPE,
					  FontFileResetFPE,
					  PrinterFontOpenFont,
					  FontFileCloseFont,
					  PrinterFontListFonts,
					  PrinterFontStartListFontsWithInfo,
					  PrinterFontListNextFontWithInfo,
					  NULL,
					  NULL,
					  NULL,
					  PrinterFontStartListFontsAndAliases,
					  PrinterFontListNextFontOrAlias,
					  FontFileEmptyBitmapSource);
}
