/* $TOG: MenuButtoP.h /main/10 1998/02/11 14:53:41 kaleb $
 *
Copyright 1989,1998  The Open Group

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
/* $XFree86: xc/lib/Xaw/MenuButtoP.h,v 1.6 1999/06/20 08:41:02 dawes Exp $ */

/*
 * MenuButtonP.h - Private Header file for MenuButton widget.
 *
 * This is the private header file for the Athena MenuButton widget.
 * It is intended to provide an easy method of activating pulldown menus.
 *
 * Date:    May 2, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#ifndef _XawMenuButtonP_h
#define _XawMenuButtonP_h

#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/CommandP.h>

/* New fields for the MenuButton widget class */
typedef struct _MenuButtonClass {
    XtPointer extension;
} MenuButtonClassPart;

/* class record declaration */
typedef struct _MenuButtonClassRec {
    CoreClassPart	    core_class;
    SimpleClassPart	    simple_class;
    LabelClassPart	    label_class;
    CommandClassPart	    command_class;
    MenuButtonClassPart	    menuButton_class;
} MenuButtonClassRec;

extern MenuButtonClassRec menuButtonClassRec;

/* New fields for the MenuButton widget */
typedef struct {
    /* resources */
    String menu_name;
#ifndef OLDXAW
    XtPointer pad[4];	/* for future use and keep binary compatability */
#endif
} MenuButtonPart;

/* widget declaration */
typedef struct _MenuButtonRec {
    CorePart         core;
    SimplePart	     simple;
    LabelPart	     label;
    CommandPart	     command;
    MenuButtonPart   menu_button;
} MenuButtonRec;

#endif /* _XawMenuButtonP_h */
