.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/void/void.cpp,v 1.1 2000/03/03 01:05:50 dawes Exp $ 
.TH VOID __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
void \- null input driver
.SH SYNOPSIS
.B "Section ""InputDevice"""
.br
.BI "  Identifier """ idevname """"
.br
.B  "  Driver ""void"""
.br
\ \ ...
.br
.B EndSection
.SH DESCRIPTION
.B void 
is an dummy/null XFree86 input driver.  It doesn't connect to any
physical device, and it never delivers any events.  It functions as
both a pointer and keyboard device, and may be used as X server's core
pointer and/or core keyboard.  It's purpose is to allow the X server
to operate without a core pointer and/or core keyboard.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
driver doesn't have any configuration options in addition to those.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1).
.SH AUTHORS
Authors include...
