/* $TOG: StrToWidg.c /main/11 1998/02/06 15:46:07 kaleb $ */

/*

Copyright 1994, 1998  The Open Group

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
/* $XFree86: xc/lib/Xmu/StrToWidg.c,v 1.5 1998/10/03 09:06:36 dawes Exp $ */

/*
 * XmuCvtStringToWidget
 *
 * static XtConvertArgRec parentCvtArgs[] = {
 *   {XtBaseOffset, (XtPointer)XtOffset(Widget, core.parent), sizeof(Widget)},
 * };
 *
 * matches the string against the name of the immediate children (normal
 * or popup) of the parent.  If none match, compares string to classname
 * & returns first match.  Case is significant.
 */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ObjectP.h>
#include <X11/Xmu/Converters.h>

#define	done(address, type) \
{						\
  toVal->size = sizeof(type);			\
  toVal->addr = (XPointer)address;		\
	  return; \
}

/*ARGSUSED*/
void
XmuCvtStringToWidget(XrmValuePtr args, Cardinal *num_args,
		     XrmValuePtr fromVal, XrmValuePtr toVal)
{
    static Widget widget, *widgetP, parent;
    XrmName name = XrmStringToName(fromVal->addr);
    int i;

    if (*num_args != 1)
	XtErrorMsg("wrongParameters", "cvtStringToWidget", "xtToolkitError",
		   "StringToWidget conversion needs parent arg", NULL, 0);

    parent = *(Widget*)args[0].addr;
    /* try to match names of normal children */
  if (XtIsComposite(parent))
    {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	   i; i--, widgetP++)
	if ((*widgetP)->core.xrm_name == name)
	  {
		widget = *widgetP;
		done(&widget, Widget);
	    }
	}

    /* try to match names of popup children */
    i = parent->core.num_popups;
  for (widgetP = parent->core.popup_list; i; i--, widgetP++)
    if ((*widgetP)->core.xrm_name == name)
      {
	    widget = *widgetP;
	    done(&widget, Widget);
	}

    /* try to match classes of normal children */
  if (XtIsComposite(parent))
    {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	   i; i--, widgetP++)
	if ((*widgetP)->core.widget_class->core_class.xrm_class == name)
	  {
		widget = *widgetP;
		done(&widget, Widget);
	    }
	}

    /* try to match classes of popup children */
    i = parent->core.num_popups;
  for (widgetP = parent->core.popup_list; i; i--, widgetP++)
    if ((*widgetP)->core.widget_class->core_class.xrm_class == name)
      {
	    widget = *widgetP;
	    done(&widget, Widget);
	}

    XtStringConversionWarning(fromVal->addr, XtRWidget);
    toVal->addr = NULL;
    toVal->size = 0;
}

#undef done

#define	newDone(type, value) \
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XtPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}


/*ARGSUSED*/
Boolean
XmuNewCvtStringToWidget(Display *dpy, XrmValue *args, Cardinal *num_args,
			XrmValue *fromVal, XrmValue *toVal,
			XtPointer *converter_data)
{
    Widget *widgetP, parent;
    XrmName name = XrmStringToName(fromVal->addr);
    int i;

    if (*num_args != 1)
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
			"wrongParameters","cvtStringToWidget","xtToolkitError",
			"String To Widget conversion needs parent argument",
			(String *)NULL, (Cardinal *)NULL);

    parent = *(Widget*)args[0].addr;
    /* try to match names of normal children */
  if (XtIsComposite(parent))
    {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	   i; i--, widgetP++)
	    if ((*widgetP)->core.xrm_name == name)
		newDone(Widget, *widgetP);
	}

    /* try to match names of popup children */
    i = parent->core.num_popups;
  for (widgetP = parent->core.popup_list; i; i--, widgetP++)
	if ((*widgetP)->core.xrm_name == name)
	    newDone(Widget, *widgetP);

    /* try to match classes of normal children */
  if (XtIsComposite(parent))
    {
	i = ((CompositeWidget)parent)->composite.num_children;
	for (widgetP = ((CompositeWidget)parent)->composite.children;
	   i; i--, widgetP++)
	    if ((*widgetP)->core.widget_class->core_class.xrm_class == name)
		newDone(Widget, *widgetP);
	}

    /* try to match classes of popup children */
    i = parent->core.num_popups;
  for (widgetP = parent->core.popup_list; i; i--, widgetP++)
	if ((*widgetP)->core.widget_class->core_class.xrm_class == name)
	    newDone(Widget, *widgetP);

    XtDisplayStringConversionWarning(dpy, (String)fromVal->addr, XtRWidget);
  return (False);
}

/*ARGSUSED*/
Boolean
XmuCvtWidgetToString(Display *dpy, XrmValuePtr args, Cardinal *num_args,
		     XrmValuePtr fromVal, XrmValuePtr toVal,
		     XtPointer *data)
{
  static String buffer;
  Cardinal size;
  Widget widget;

  widget = *(Widget *)fromVal->addr;

  if (widget)
    buffer = XrmQuarkToString(widget->core.xrm_name);
  else
    buffer = "(null)";

  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
    {
      if (toVal->size < size)
	{
	  toVal->size = size;
	  return (False);
	}
      strcpy((char *)toVal->addr, buffer);
    }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = sizeof(String);

  return (True);
}
