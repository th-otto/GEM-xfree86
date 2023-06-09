/* $TOG: Dialog.h /main/7 1998/02/09 13:40:51 kaleb $ */
/*

Copyright 1989, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/bitmap/Dialog.h,v 1.2 1999/02/01 11:56:08 dawes Exp $ */

/*
 * Author:  Davor Matic, MIT X Consortium
 */


/*#define None   0*/
#define Yes    1<<1
#define No     1<<2
#define Maybe  1<<3  /* :-) */
#define Okay   1<<4
#define Abort  1<<5
#define Cancel 1<<6
#define Retry  1<<7

typedef struct {
  Widget top_widget, shell_widget, dialog_widget;
  unsigned long options;
} _Dialog, *Dialog;

typedef struct {
    String name;
    unsigned long flag;
} DialogButton;

extern Dialog CreateDialog(Widget, String, unsigned long);
extern unsigned long PopupDialog(Dialog, String, String, String *, XtGrabKind);
extern void PopdownDialog(Dialog, String *);
