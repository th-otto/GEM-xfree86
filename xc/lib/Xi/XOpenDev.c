/* $TOG: XOpenDev.c /main/11 1998/02/06 15:04:22 kaleb $ */

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
/* $XFree86: xc/lib/Xi/XOpenDev.c,v 3.1 1998/10/03 09:06:11 dawes Exp $ */

/***********************************************************************
 *
 * XOpenDevice - Request the server to open and extension input device.
 *
 */

#include "XI.h"
#include "XIproto.h"
#include "Xlibint.h"
#include "XInput.h"
#include "extutil.h"
#include "XIint.h"

XDevice
*XOpenDevice(dpy, id)
    register Display 	*dpy;
    register XID	id;
    {	
    register long	rlen; /* raw length */
    xOpenDeviceReq 	*req;
    xOpenDeviceReply 	rep;
    XDevice 		*dev;
    XExtDisplayInfo *info = XInput_find_display (dpy);

    LockDisplay (dpy);
    if (_XiCheckExtInit(dpy, XInput_Initial_Release) == -1)
	return ((XDevice *) NoSuchExtension);

    GetReq(OpenDevice,req);		
    req->reqType = info->codes->major_opcode;
    req->ReqType = X_OpenDevice;
    req->deviceid = id;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) 
	{
	UnlockDisplay(dpy);
	SyncHandle();
	return (XDevice *) NULL;
	}

    rlen = rep.length << 2;
    dev = (XDevice *) Xmalloc (sizeof(XDevice) + rep.num_classes * 
	sizeof (XInputClassInfo));
    if (dev)
	{
	int dlen; /* data length */

	dev->device_id = req->deviceid;
	dev->num_classes = rep.num_classes;
	dev->classes = (XInputClassInfo *) ((char *) dev + sizeof (XDevice));
	dlen = rep.num_classes * sizeof(xInputClassInfo);
	_XRead (dpy, (char *)dev->classes, dlen);
	/* could be padding that we still need to eat (yummy!) */
	if(rlen - dlen > 0)
	    _XEatData (dpy, (unsigned long) rlen - dlen);
	}
    else
	_XEatData (dpy, (unsigned long) rlen);

    UnlockDisplay (dpy);
    SyncHandle();
    return (dev);
    }

