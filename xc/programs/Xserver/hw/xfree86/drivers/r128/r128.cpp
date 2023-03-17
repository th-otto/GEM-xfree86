.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/r128/r128.cpp,v 1.3 2000/03/06 22:59:26 dawes Exp $ 
.TH R128 __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
r128 \- ATI Rage 128 video driver
.SH SYNOPSIS
.B "Section ""Device"""
.br
.BI "  Identifier """  devname """"
.br
.B  "  Driver ""r128"""
.br
\ \ ...
.br
.B EndSection
.SH DESCRIPTION
.B r128
is an XFree86 driver for ATI Rage 128 based video cards.  It contains
full support for 8, 15, 16 and 24 bit pixel depths, hardware
acceleration of drawing primitives, hardware cursor, video modes up to
1800x1440 @ 70Hz, doublescan modes (e.g., 320x200 and 320x240), gamma
correction at all pixel depths, a fully programming dot clock and robust
text mode restoration for VT switching.
.SH SUPPORTED HARDWARE
The
.B r128
driver supports all ATI Rage 128 based video cards including the Rage
Fury AGP 32MB, the XPERT 128 AGP 16MB and the XPERT 99 AGP 8MB.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects all device information necessary to initialize
the card.  However, if you have problems with auto-detection, you can
specify:
.PP
.RS 4
VideoRam - in kilobytes
.br
MemBase  - physical address of the linear framebuffer
.br
IOBase   - physical address of the MMIO registers
.br
ChipID   - PCI DEVICE ID
.RE
.PP
In addition, the following driver
.B Options
are supported:
.TP
.BI "Option ""SWcursor"" """ boolean """
Selects software cursor.  The default is
.B off.
.TP
.BI "Option ""NoAccel"" """ boolean """
Enables or disables all hardware acceleration.  The default is to
.B enable
hardware acceleration.
.TP
.BI "Option ""Dac6Bit"" """ boolean """
Enables or disables the use of 6 bits per color component when in 8 bpp
mode (emulates VGA mode).  By default, all 8 bits per color component
are used.  The default is
.B off.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1)
.SH AUTHORS
.nf
Rickard E. (Rik) Faith   \fIfaith@precisioninsight.com\fP
Kevin E. Martin          \fIkevin@precisioninsight.com\fP
