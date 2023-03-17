/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Vendor.c,v 1.5 2000/01/26 02:00:51 alanh Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

/* View/edit this file with tab stops set to 4 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static xf86ConfigSymTabRec VendorTab[] =
{
	{COMMENT, "###"},
	{ENDSECTION, "endsection"},
	{IDENTIFIER, "identifier"},
	{OPTION, "option"},
	{-1, ""},
};

#define CLEANUP freeVendorList

XF86ConfVendorPtr
parseVendorSection (void)
{
	int has_ident = FALSE;
	parsePrologue (XF86ConfVendorPtr, XF86ConfVendorRec)

	while ((token = xf86GetToken (VendorTab)) != ENDSECTION)
	{
		switch (token)
		{
		case COMMENT:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "###");
			ptr->vnd_comment = val.str;
			break;
		case IDENTIFIER:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Identifier");
			ptr->vnd_identifier = val.str;
			has_ident = TRUE;
			break;
		case OPTION:
			{
				char *name;
				if ((token = xf86GetToken (NULL)) != STRING)
					Error (BAD_OPTION_MSG, NULL);
				name = val.str;
				if ((token = xf86GetToken (NULL)) == STRING)
				{
					ptr->vnd_option_lst = addNewOption (ptr->vnd_option_lst,
														name, val.str);
				}
				else
				{
					ptr->vnd_option_lst = addNewOption (ptr->vnd_option_lst,
														name, NULL);
					xf86UnGetToken (token);
				}
			}
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86TokenString ());
			break;
		}

	}

	if (!has_ident)
		Error (NO_IDENT_MSG, NULL);

#ifdef DEBUG
	printf ("Vendor section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
printVendorSection (FILE * cf, XF86ConfVendorPtr ptr)
{
	XF86OptionPtr optr;

	while (ptr)
	{
		fprintf (cf, "Section \"Vendor\"\n");
		if (ptr->vnd_comment)
			fprintf (cf, "\t###            \"%s\"\n", ptr->vnd_comment);
		if (ptr->vnd_identifier)
			fprintf (cf, "\tIdentifier     \"%s\"\n", ptr->vnd_identifier);

		for (optr = ptr->vnd_option_lst; optr; optr = optr->list.next)
		{
			fprintf (cf, "\tOption      \"%s\"", optr->opt_name);
			if (optr->opt_val)
				fprintf (cf, " \"%s\"", optr->opt_val);
			fprintf (cf, "\n");
		}
		fprintf (cf, "EndSection\n\n");
		ptr = ptr->list.next;
	}
}

void
freeVendorList (XF86ConfVendorPtr p)
{
	if (p == NULL)
		return;
	TestFree (p->vnd_identifier);
	OptionListFree (p->vnd_option_lst);
	xf86conffree (p);
}

XF86ConfVendorPtr
xf86FindVendor (const char *name, XF86ConfVendorPtr list)
{
    while (list)
    {
        if (NameCompare (list->vnd_identifier, name) == 0)
            return (list);
        list = list->list.next;
    }
    return (NULL);
}

