/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/read.c,v 1.11 1999/09/04 13:04:54 dawes Exp $ */
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

static xf86ConfigSymTabRec TopLevelTab[] =
{
	{SECTION, "section"},
	{-1, ""},
};

#define CLEANUP XF86FreeConfig

XF86ConfigPtr
xf86ReadConfigFile (void)
{
	int token;
	XF86ConfigPtr ptr = NULL;

	if ((ptr = xf86confmalloc (sizeof (XF86ConfigRec))) == NULL)
	{
		return NULL;
	}
	memset (ptr, 0, sizeof (XF86ConfigRec));

	while ((token = xf86GetToken (TopLevelTab)) != EOF_TOKEN)
	{
		switch (token)
		{
		case SECTION:
			if (xf86GetToken (NULL) != STRING)
			{
				xf86ParseError (QUOTE_MSG, "Section");
				CLEANUP (ptr);
				return (NULL);
			}
			SetSection (val.str);
			if (NameCompare (val.str, "files") == 0)
			{
				HANDLE_RETURN (conf_files, parseFilesSection ());
			}
			else if (NameCompare (val.str, "serverflags") == 0)
			{
				HANDLE_RETURN (conf_flags, parseFlagsSection ());
			}
			else if (NameCompare (val.str, "keyboard") == 0)
			{
				HANDLE_LIST (conf_input_lst, parseKeyboardSection,
							 XF86ConfInputPtr);
			}
			else if (NameCompare (val.str, "pointer") == 0)
			{
				HANDLE_LIST (conf_input_lst, parsePointerSection,
							 XF86ConfInputPtr);
			}
			else if (NameCompare (val.str, "videoadaptor") == 0)
			{
				HANDLE_LIST (conf_videoadaptor_lst, parseVideoAdaptorSection,
							 XF86ConfVideoAdaptorPtr);
			}
			else if (NameCompare (val.str, "device") == 0)
			{
				HANDLE_LIST (conf_device_lst, parseDeviceSection,
							 XF86ConfDevicePtr);
			}
			else if (NameCompare (val.str, "monitor") == 0)
			{
				HANDLE_LIST (conf_monitor_lst, parseMonitorSection,
							 XF86ConfMonitorPtr);
			}
			else if (NameCompare (val.str, "modes") == 0)
			{
				HANDLE_LIST (conf_modes_lst, parseModesSection,
							 XF86ConfModesPtr);
			}
			else if (NameCompare (val.str, "screen") == 0)
			{
				HANDLE_LIST (conf_screen_lst, parseScreenSection,
							 XF86ConfScreenPtr);
			}
			else if (NameCompare(val.str, "inputdevice") == 0)
			{
				HANDLE_LIST (conf_input_lst, parseInputSection,
							 XF86ConfInputPtr);
			}
			else if (NameCompare (val.str, "module") == 0)
			{
				HANDLE_RETURN (conf_modules, parseModuleSection ());
			}
			else if (NameCompare (val.str, "serverlayout") == 0)
			{
				HANDLE_LIST (conf_layout_lst, parseLayoutSection,
							 XF86ConfLayoutPtr);
			}
			else if (NameCompare (val.str, "vendor") == 0)
			{
				HANDLE_LIST (conf_vendor_lst, parseVendorSection,
							 XF86ConfVendorPtr);
			}
			else if (NameCompare (val.str, "dri") == 0)
			{
				HANDLE_RETURN (conf_dri, parseDRISection ());
			}
			else
			{
				Error (INVALID_SECTION_MSG, xf86TokenString ());
			}
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86TokenString ());
		}
	}

	if (validateConfig (ptr))
		return (ptr);
	else
	{
		CLEANUP (ptr);
		return (NULL);
	}
}

#undef CLEANUP

/* 
 * This function resolves name references and reports errors if the named
 * objects cannot be found.
 */
int
validateConfig (XF86ConfigPtr p)
{
	if (!validateDevice (p))
		return FALSE;
	if (!validateScreen (p))
		return FALSE;
	if (!validateInput (p))
		return FALSE;
	if (!validateLayout (p))
		return FALSE;

	return (TRUE);
}

/* 
 * adds an item to the end of the linked list. Any record whose first field
 * is a GenericListRec can be cast to this type and used with this function.
 * A pointer to the head of the list is returned to handle the addition of
 * the first item.
 */
GenericListPtr
addListItem (GenericListPtr head, GenericListPtr new)
{
	GenericListPtr p = head;
	GenericListPtr last = NULL;

	while (p)
	{
		last = p;
		p = p->next;
	}

	if (last)
	{
		last->next = new;
		return (head);
	}
	else
		return (new);
}

void
XF86FreeConfig (XF86ConfigPtr p)
{
	if (p == NULL)
		return;

	freeFiles (p->conf_files);
	freeModules (p->conf_modules);
	freeFlags (p->conf_flags);
	freeMonitorList (p->conf_monitor_lst);
	freeVideoAdaptorList (p->conf_videoadaptor_lst);
	freeDeviceList (p->conf_device_lst);
	freeScreenList (p->conf_screen_lst);
	freeLayoutList (p->conf_layout_lst);
	freeInputList (p->conf_input_lst);

	xf86conffree (p);
}
