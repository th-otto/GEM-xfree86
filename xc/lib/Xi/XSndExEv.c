/* $TOG: XSndExEv.c /main/8 1998/02/06 15:05:03 kaleb $ */

/************************************************************

Copyright 1989, 1998  The Open Group

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

Copyright 1989 by Hewlett-Packard Company, Palo Alto, California.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Hewlett-Packard not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

HEWLETT-PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
HEWLETT-PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

********************************************************/
/* $XFree86: xc/lib/Xi/XSndExEv.c,v 3.1 1998/10/03 09:06:14 dawes Exp $ */

/***********************************************************************
 *
 * XSendExtensionEvent - send an extension event to a client.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"
#include "XIint.h"

extern Status _XiEventToWire();

Status
XSendExtensionEvent (dpy, dev, dest, prop, count, list, event)
    register Display 	*dpy;
    XDevice 		*dev;
    Window 		dest;
    Bool		prop;
    int			count;
    XEventClass		*list;
    XEvent		*event;
    {       
    int				num_events;
    int				ev_size;
    xSendExtensionEventReq 	*req;
    xEvent 			*ev;
    register Status 		(**fp)();
    Status 			status;
    XExtDisplayInfo *info = XInput_find_display (dpy);

    LockDisplay (dpy);
    if (_XiCheckExtInit(dpy, XInput_Initial_Release) == -1)
	return (NoSuchExtension);

    /* call through display to find proper conversion routine */

    fp = &dpy->wire_vec[event->type & 0177];
    if (*fp == NULL) 
	*fp = _XiEventToWire;
    status = (**fp)(dpy, event, &ev, &num_events);

    if (status) 
	{
	GetReq(SendExtensionEvent,req);		
        req->reqType = info->codes->major_opcode;
	req->ReqType = X_SendExtensionEvent;
	req->deviceid = dev->device_id;
	req->destination = dest;
	req->propagate = prop;
	req->count = count;
	req->num_events = num_events;
	ev_size = num_events * sizeof (xEvent);
	req->length += (count + (ev_size >> 2));

	/* note: Data is a macro that uses its arguments multiple
           times, so "count" is changed in a separate assignment
           statement.  Any extra events must be sent before the event
	   list, in order to ensure quad alignment. */

	Data (dpy, (char *) ev, ev_size);

	count <<= 2;
	Data32 (dpy, (long *) list, count);
	XFree ((char *)ev);
	}

    UnlockDisplay(dpy);
    SyncHandle();
    return (status);
    }

