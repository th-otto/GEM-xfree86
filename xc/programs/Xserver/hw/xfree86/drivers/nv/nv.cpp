.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv.cpp,v 1.6 2000/03/03 01:05:40 dawes Exp $ 
.TH NV __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
nv \-NVIDIA video driver
.SH SYNOPSIS
.B "Section ""Device"""
.br
.BI "  Identifier """  devname """"
.br
.B  "  Driver ""nv"""
.br
\ \ ...
.br
.B EndSection
.SH DESCRIPTION
.B nv 
is an XFree86 driver for NVIDIA video cards.  The driver is fully
accelerated, and provides support for the following framebuffer depths:
8, 15, 16 (except Riva128) and 24.  All
visual types are supported for depth 8, TrueColor
visuals are supported for the other depths.  Multi-head configurations
are supported.
.SH SUPPORTED HARDWARE
The
.B nv
driver supports PCI and AGP video cards based on the following NVIDIA chips:
.TP 22
.B RIVA 128
NV3
.TP 22
.B RIVA TNT
NV4
.TP 22
.B RIVA TNT2
NV5
.TP 22
.B GeForce 256, QUADRO 
NV10
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects the chipset type and the amount of video memory
present for all chips.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option ""HWCursor"" """ boolean """
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option ""NoAccel"" """ boolean """
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option ""UseFBDev"" """ boolean """
Enable or disable use of on OS-specific fb interface (and is not supported
on all OSs).  See fbdevhw(__drivermansuffix__) for further information.
Default: off.
.TP
.BI "Option ""Rotate"" ""CW""
.TP
.BI "Option ""Rotate"" ""CCW""
Rotate the display clockwise or counterclockwise.  This mode is unaccelerated.
Default: no rotation.
.TP
.BI "Option ""ShadowFB"" """ boolean """
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(4) for further information.  Default: off.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1)
.SH AUTHORS
Authors include: David McKay, Jarno Paananen, Chas Inman, Dave Schmenk, 
Mark Vojkovich
