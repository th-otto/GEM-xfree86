/* $TOG: xtest1imp.c /main/4 1998/02/10 13:11:24 kaleb $ */
/*
 *	File: xtest1dd.c
 *
 *	This file contains the device dependent parts of the input
 *	synthesis extension.
 */

/*


Copyright 1986, 1987, 1988, 1998  The Open Group

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


Copyright 1986, 1987, 1988 by Hewlett-Packard Corporation

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Hewlett-Packard not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

Hewlett-Packard makes no representations about the 
suitability of this software for any purpose.  It is provided 
"as is" without express or implied warranty.

This software is not subject to any license of the American
Telephone and Telegraph Company or of the Regents of the
University of California.

*/

/***************************************************************
 * include files
 ***************************************************************/

#define	NEED_EVENTS
#define	NEED_REPLIES

#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#define  XTestSERVER_SIDE
#include "xtestext1.h"	
/*
 * the following include files are specific to HP's implementation
 * of the extension.  Your implementation may vary.
 */
#include "hildef.h"
#include "hpext.h"
#include "XHPproto.h"

/*
 * The following externs are specific to HP's implementation
 * of the extension.  Your implementation may vary.
 */
extern ScreenInfo		screenInfo;
extern InputInfo		inputInfo;
extern HPInputDevice		*hpPointer, *hpKeyboard;

/******************************************************************************
 *
 *	XTestGetPointerPos
 *
 * Return the position of the mouse.
 *
 */
void
XTestGetPointerPos(fmousex, fmousey)
	short *fmousex, *fmousey;
	{
	*fmousex = hpPointer->coords[0];
	*fmousey = hpPointer->coords[1];
	}

/******************************************************************************
 *
 *	XTestJumpPointer
 *
 *	Tell the server to move the mouse.
 *
 *	This is implementation-dependent.  Your implementation may vary.
 */
void
XTestJumpPointer(jx, jy, dev_type)
/*
 * the x and y position to move the mouse to
 */
int	jx;
int	jy;
/*
 * which device is supposed to move (ignored)
 */
int	dev_type;
{
	int			xdiff, screensize;
	ScreenPtr		pScreen = hpPointer->pScreen;
        xEvent			*format_ev(), *ev;
	extern			xHPEvent xE;
	int			coords[MAX_AXES];

	/*
	 * move the mouse.
	 * The kludge below is an attempt to make it possible to 
	 * test stacked screens mode.  Xtm records absolute screen
	 * positions, so we have trouble knowing whether or not the
	 * screen changed.  We make an arbitrary assumption here that
	 * if we moved more than 500 pixels in the x direction that
	 * we must have wrapped from one screen to another.  This is
	 * a fairly safe assumption unless someone set the mouse
	 * acceleration to some unreasonably large number.
	 *
	 * In any case, translate the absolute postions into a relative
	 * move from the current pointer position, and pass that
	 * relative move to process_motion.
	 */
	xdiff = jx - hpPointer->coords[0];
	if (abs(xdiff) > 500 && screenInfo.numScreens > 1)
	    {
	    if (xdiff > 0)
		{
		if (pScreen->myNum != 0)
		    screensize = screenInfo.screens[pScreen->myNum-1]->width;
		else
		    screensize = screenInfo.screens[screenInfo.numScreens-1]->width;
		xdiff -= screensize;
		}
	    else
		xdiff += pScreen->width;
	    }
	coords[0] = xdiff;
	coords[1] = jy - hpPointer->coords[1];
	process_motion (inputInfo.pointer, hpPointer, hpPointer, coords);
	ev = format_ev (inputInfo.pointer, MotionNotify, 0, GetTimeInMillis(), hpPointer, NULL);
	ProcessInputEvents();
}

/******************************************************************************
 *
 *	XTestGenerateEvent
 *
 *	Send a key/button input action to the server to be processed.
 *
 *	This is implementation-dependent.  Your implementation may vary.
 */
void
XTestGenerateEvent(dev_type, keycode, keystate, mousex, mousey)
/*
 * which device supposedly performed the action
 */
int	dev_type;
/*
 * which key/button moved
 */
int	keycode;
/*
 * whether the key/button was up or down
 */
int	keystate;
/*
 * the x and y position of the locator when the action happenned
 */
int	mousex;
int	mousey;
{
	DeviceIntPtr		dev;
	HPInputDevice		*tmp_ptr;
        xEvent			*format_ev(), *ev;

	/*
	 * the server expects to have the x and y position of the locator
	 * when the action happened placed in hpPointer.
	 */
	if (dev_type == MOUSE)
	{
		dev = inputInfo.pointer;
		hpPointer->coords[0] = mousex;
		hpPointer->coords[1] = mousey;
		tmp_ptr = hpPointer;
	}
	else
	{
		dev = inputInfo.keyboard;
		hpPointer->coords[0] = mousex;
		hpPointer->coords[1] = mousey;
		tmp_ptr = hpKeyboard;
	}
	/*
	 * convert the keystate back into server-dependent state values
	 */
	if (keycode < 8 )
	{
		/*
		 * if keycode < 8, this is really a button. 
		 */
		if (keystate == XTestKEY_UP)
		{
			keystate = ButtonRelease;
		}
		else
		{
			keystate = ButtonPress;
		}
	}
	else
	{
		if (keystate == XTestKEY_UP)
		{
			keystate = KeyRelease;
		}
		else
		{
			keystate = KeyPress;
		}
	}
	/*
	 * Tell the server to process all of the events in its input queue.
	 * This makes sure that there is room in the server's input queue
	 * for a key/button input event.
	 */
	ProcessInputEvents();
	/*
	 * put a key/button input action into the servers input event queue
	 */
	ev = format_ev (dev, keystate, keycode, GetTimeInMillis(), tmp_ptr, NULL);
	/*
	 * Tell the server to process all of the events in its input queue.
	 * This makes sure that key/button event we just put in the queue
	 * is processed immediately.
	 */
	ProcessInputEvents();
}

/******************************************************************************
 *
 *	check_for_motion_steal
 *
 *	Called from xosMoveMouse.
 */

check_for_motion_steal (hotX, hotY)
    register int hotX, hotY;
    {
#ifdef XTESTEXT1
    extern int	on_steal_input; 		/* defined in xtestext1di.c */
    extern short	xtest_mousex; 		/* defined in xtestext1di.c */
    extern short	xtest_mousey; 		/* defined in xtestext1di.c */

    if ((on_steal_input) &&
	((hotX != xtest_mousex) || (hotY != xtest_mousey))) /* mouse moved    */
	{
	XTestStealMotionData((hotX - xtest_mousex),
	     (hotY - xtest_mousey),
	     MOUSE,
	     xtest_mousex,
	     xtest_mousey);
	}
#endif /* XTESTEXT1 */
    }

