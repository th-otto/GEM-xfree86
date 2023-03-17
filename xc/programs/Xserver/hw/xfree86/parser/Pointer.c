/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Pointer.c,v 1.7 1999/09/04 13:04:54 dawes Exp $ */
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

static xf86ConfigSymTabRec PointerTab[] =
{
	{PROTOCOL, "protocol"},
	{EMULATE3, "emulate3buttons"},
	{EM3TIMEOUT, "emulate3timeout"},
	{ENDSUBSECTION, "endsubsection"},
	{ENDSECTION, "endsection"},
	{PDEVICE, "device"},
	{PDEVICE, "port"},
	{BAUDRATE, "baudrate"},
	{SAMPLERATE, "samplerate"},
	{CLEARDTR, "cleardtr"},
	{CLEARRTS, "clearrts"},
	{CHORDMIDDLE, "chordmiddle"},
	{PRESOLUTION, "resolution"},
	{DEVICE_NAME, "devicename"},
	{ALWAYSCORE, "alwayscore"},
	{PBUTTONS, "buttons"},
	{ZAXISMAPPING, "zaxismapping"},
	{-1, ""},
};

static xf86ConfigSymTabRec ZMapTab[] =
{
	{XAXIS, "x"},
	{YAXIS, "y"},
	{-1, ""},
};

#define CLEANUP freeInputList

XF86ConfInputPtr
parsePointerSection (void)
{
	char *s, *s1, *s2;
	int l;
	parsePrologue (XF86ConfInputPtr, XF86ConfInputRec)

	while ((token = xf86GetToken (PointerTab)) != ENDSECTION)
	{
		switch (token)
		{
		case PROTOCOL:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Protocol");
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("Protocol"),
												val.str);
			break;
		case PDEVICE:
			if (xf86GetToken (NULL) != STRING)
				Error (QUOTE_MSG, "Device");
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("Device"),
												val.str);
			break;
		case EMULATE3:
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("Emulate3Buttons"),
												NULL);
			break;
		case EM3TIMEOUT:
			if (xf86GetToken (NULL) != NUMBER || val.num < 0)
				Error (POSITIVE_INT_MSG, "Emulate3Timeout");
			s = ULongToString(val.num);
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("Emulate3Timeout"),
												s);
			break;
		case CHORDMIDDLE:
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("ChordMiddle"),
												NULL);
			break;
		case PBUTTONS:
			if (xf86GetToken (NULL) != NUMBER || val.num < 0)
				Error (POSITIVE_INT_MSG, "Buttons");
			s = ULongToString(val.num);
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("Buttons"), s);
			break;
		case BAUDRATE:
			if (xf86GetToken (NULL) != NUMBER || val.num < 0)
				Error (POSITIVE_INT_MSG, "BaudRate");
			s = ULongToString(val.num);
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("BaudRate"), s);
			break;
		case SAMPLERATE:
			if (xf86GetToken (NULL) != NUMBER || val.num < 0)
				Error (POSITIVE_INT_MSG, "SampleRate");
			s = ULongToString(val.num);
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("SampleRate"), s);
			break;
		case PRESOLUTION:
			if (xf86GetToken (NULL) != NUMBER || val.num < 0)
				Error (POSITIVE_INT_MSG, "Resolution");
			s = ULongToString(val.num);
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("Resolution"), s);
			break;
		case CLEARDTR:
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("ClearDTR"), NULL);
			break;
		case CLEARRTS:
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("ClearRTS"), NULL);
			break;
		case ZAXISMAPPING:
			switch (xf86GetToken(ZMapTab)) {
			case NUMBER:
				if (val.num < 0)
					Error (ZAXISMAPPING_MSG, NULL);
				s1 = ULongToString(val.num);
				if (xf86GetToken (NULL) != NUMBER || val.num < 0)
					Error (ZAXISMAPPING_MSG, NULL);
				s2 = ULongToString(val.num);
				l = strlen(s1) + 1 + strlen(s2) + 1;
				s = xf86confmalloc(l);
				sprintf(s, "%s %s", s1, s2);
				xf86conffree(s1);
				xf86conffree(s2);
				break;
			case XAXIS:
				s = ConfigStrdup("x");
				break;
			case YAXIS:
				s = ConfigStrdup("y");
				break;
			default:
				Error (ZAXISMAPPING_MSG, NULL);
				break;
			}
			ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
												ConfigStrdup("ZAxisMapping"),
												s);
			break;
		case ALWAYSCORE:
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86TokenString ());
			break;
		}
	}

	ptr->inp_identifier = ConfigStrdup(CONF_IMPLICIT_POINTER);
	ptr->inp_driver = ConfigStrdup("mouse");
	ptr->inp_option_lst = addNewOption(ptr->inp_option_lst,
										ConfigStrdup("CorePointer"), NULL);

#ifdef DEBUG
	printf ("Pointer section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

