/* $TOG: XPanoramiX.c /main/2 1997/11/16 08:45:41 kaleb $ */
/****************************************************************
*                                                               *
*    Copyright (c) Digital Equipment Corporation, 1991, 1997    *
*                                                               *
*   All Rights Reserved.  Unpublished rights  reserved  under   *
*   the copyright laws of the United States.                    *
*                                                               *
*   The software contained on this media  is  proprietary  to   *
*   and  embodies  the  confidential  technology  of  Digital   *
*   Equipment Corporation.  Possession, use,  duplication  or   *
*   dissemination of the software and media is authorized only  *
*   pursuant to a valid written license from Digital Equipment  *
*   Corporation.                                                *
*                                                               *
*   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
*   by the U.S. Government is subject to restrictions  as  set  *
*   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
*   or  in  FAR 52.227-19, as applicable.                       *
*                                                               *
*****************************************************************/
/* $XFree86: xc/lib/Xinerama/Xinerama.c,v 1.1 2000/02/27 23:10:04 mvojkovi Exp $ */

#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include "Xext.h"			/* in ../include */
#include "extutil.h"			/* in ../include */
#include "panoramiXext.h"
#include "panoramiXproto.h"		/* in ../include */
#include "Xinerama.h"


static XExtensionInfo _panoramiX_ext_info_data;
static XExtensionInfo *panoramiX_ext_info = &_panoramiX_ext_info_data;
static /* const */ char *panoramiX_extension_name = PANORAMIX_PROTOCOL_NAME;

#define PanoramiXCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, panoramiX_extension_name, val)
#define PanoramiXSimpleCheckExtension(dpy,i) \
  XextSimpleCheckExtension (dpy, i, panoramiX_extension_name)

static int close_display();
static /* const */ XExtensionHooks panoramiX_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL,				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, panoramiX_ext_info,
				   panoramiX_extension_name, 
				   &panoramiX_extension_hooks,
				   0, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, panoramiX_ext_info)



/****************************************************************************
 *                                                                          *
 *			    PanoramiX public interfaces                         *
 *                                                                          *
 ****************************************************************************/

Bool XPanoramiXQueryExtension (
    Display *dpy,
    int *event_basep,
    int *error_basep
)
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}


Status XPanoramiXQueryVersion(
    Display *dpy,
    int	    *major_versionp, 
    int *minor_versionp
)
{
    XExtDisplayInfo *info = find_display (dpy);
    xPanoramiXQueryVersionReply	    rep;
    register xPanoramiXQueryVersionReq  *req;

    PanoramiXCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (PanoramiXQueryVersion, req);
    req->reqType = info->codes->major_opcode;
    req->panoramiXReqType = X_PanoramiXQueryVersion;
    req->clientMajor = PANORAMIX_MAJOR_VERSION;
    req->clientMinor = PANORAMIX_MINOR_VERSION;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    *major_versionp = rep.majorVersion;
    *minor_versionp = rep.minorVersion;
    UnlockDisplay (dpy);
    SyncHandle ();
    return 1;
}

XPanoramiXInfo *XPanoramiXAllocInfo(void)
{
	return (XPanoramiXInfo *) Xmalloc (sizeof (XPanoramiXInfo));
}

Status XPanoramiXGetState (
    Display		*dpy,
    Drawable		drawable,
    XPanoramiXInfo	*panoramiX_info
)
{
    XExtDisplayInfo			*info = find_display (dpy);
    xPanoramiXGetStateReply	rep;
    register xPanoramiXGetStateReq	*req;

    PanoramiXCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (PanoramiXGetState, req);
    req->reqType = info->codes->major_opcode;
    req->panoramiXReqType = X_PanoramiXGetState;
    req->window = drawable;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    panoramiX_info->window = rep.window;
    panoramiX_info->State = rep.state;
    return 1;
}

Status XPanoramiXGetScreenCount (
    Display		*dpy,
    Drawable		drawable,
    XPanoramiXInfo	*panoramiX_info
)
{
    XExtDisplayInfo			*info = find_display (dpy);
    xPanoramiXGetScreenCountReply	rep;
    register xPanoramiXGetScreenCountReq	*req;

    PanoramiXCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (PanoramiXGetScreenCount, req);
    req->reqType = info->codes->major_opcode;
    req->panoramiXReqType = X_PanoramiXGetScreenCount;
    req->window = drawable;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    panoramiX_info->window = rep.window;
    panoramiX_info->ScreenCount = rep.ScreenCount;
    return 1;
}

Status XPanoramiXGetScreenSize (
    Display		*dpy,
    Drawable		drawable,
    int			screen_num,
    XPanoramiXInfo	*panoramiX_info
)
{
    XExtDisplayInfo			*info = find_display (dpy);
    xPanoramiXGetScreenSizeReply	rep;
    register xPanoramiXGetScreenSizeReq	*req;

    PanoramiXCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (PanoramiXGetScreenSize, req);
    req->reqType = info->codes->major_opcode;
    req->panoramiXReqType = X_PanoramiXGetScreenSize;
    req->window = drawable;
    req->screen = screen_num;			/* need to define */ 
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return 0;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    panoramiX_info->window = rep.window;
    panoramiX_info->screen = rep.screen;
    panoramiX_info->width =  rep.width;
    panoramiX_info->height = rep.height;
    return 1;
}

/*******************************************************************\
  Alternate interface to make up for shortcomings in the original,
  namely, the omission of the screen origin.  The new interface is
  in the "Xinerama" namespace instead of "PanoramiX".
\*******************************************************************/

Bool XineramaQueryExtension (
   Display *dpy,
   int     *event_base,
   int     *error_base
)
{
   return XPanoramiXQueryExtension(dpy, event_base, error_base);
}

Status XineramaQueryVersion(
   Display *dpy,
   int     *major,
   int     *minor
)
{
   return XPanoramiXQueryVersion(dpy, major, minor);
}

Bool XineramaIsActive(Display *dpy)
{
    xXineramaIsActiveReply	rep;
    xXineramaIsActiveReq  	*req;
    XExtDisplayInfo 		*info = find_display (dpy);

    if(!XextHasExtension(info))
	return False;  /* server doesn't even have the extension */

    LockDisplay (dpy);
    GetReq (XineramaIsActive, req);
    req->reqType = info->codes->major_opcode;
    req->panoramiXReqType = X_XineramaIsActive;
    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return False;
    }
    UnlockDisplay (dpy);
    SyncHandle ();
    return rep.state;
}

#include <stdio.h>

XineramaScreenInfo * 
XineramaQueryScreens(
   Display *dpy,
   int     *number
)
{
    XExtDisplayInfo		*info = find_display (dpy);
    xXineramaQueryScreensReply	rep;
    xXineramaQueryScreensReq	*req;
    XineramaScreenInfo		*scrnInfo = NULL;

    PanoramiXCheckExtension (dpy, info, 0);

    LockDisplay (dpy);
    GetReq (XineramaQueryScreens, req);
    req->reqType = info->codes->major_opcode;
    req->panoramiXReqType = X_XineramaQueryScreens;
    if (!_XReply (dpy, (xReply *) &rep, 0, xFalse)) {
	UnlockDisplay (dpy);
	SyncHandle ();
	return NULL;
    }

    if(rep.number) {
	if((scrnInfo = Xmalloc(sizeof(XineramaScreenInfo) * rep.number))) {
	    xXineramaScreenInfo scratch;
	    int i;

	    for(i = 0; i < rep.number; i++) {
		_XRead(dpy, (char*)(&scratch), sz_XineramaScreenInfo);
		scrnInfo[i].screen_number = i;
		scrnInfo[i].x_org 	  = scratch.x_org;
		scrnInfo[i].y_org 	  = scratch.y_org;
		scrnInfo[i].width 	  = scratch.width;
		scrnInfo[i].height 	  = scratch.height;
	    }

	    *number = rep.number;
	} else
	    _XEatData(dpy, rep.length << 2);
    }

    UnlockDisplay (dpy);
    SyncHandle ();
    return scrnInfo;
}



