.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/rendition.cpp,v 1.2 2000/03/05 16:59:14 dawes Exp $ 
.TH RENDITION __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
rendition \- Rendition video driver
.SH SYNOPSIS
.B "Section ""Device"""
.br
.BI "  Identifier """  devname """"
.br
.B  "  Driver ""rendition"""
.br
\ \ ...
.br
.B EndSection
.SH DESCRIPTION
.B rendition 
is an XFree86 driver for Rendition/Micron based video cards.  The driver
supports following framebuffer depths: 8, 15 (Verite V1000 only), 16
and 24. Acceleration and multi-head configurations are
not supported yet, but are work in progress.
.SH SUPPORTED HARDWARE
The
.B rendition
driver supports PCI and AGP video cards based on the following Rendition/Micron chips:
.TP 12
.B V1000
Verite V1000 based cards.
.TP 12
.B V2100
Verite V2100 based cards. Diamond Stealth II S220 is the only known such card.
.TP 12
.B V2200
Verite V2200 based cards.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects the chipset type, but the following
.B ChipSet
names may optionally be specified in the config file
.B """Device"""
section, and will override the auto-detection:
.PP
.RS 4
"v1000", "v2100", "v2200".
.RE
.PP
The driver will auto-detect the amount of video memory present for all
chips. If the amount of memory is detected incorrectly, the actual amount
of video memory should be specified with a
.B VideoRam
entry in the config file
.B """Device"""
section.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option ""SWCursor"" """ boolean """
Disables use of the hardware cursor. Default: use HW-cursor.
.TP
.BI "Option ""OverclockMem"" """ boolean """
Increases the Mem/Sys clock to 125MHz/60MHz from standard 110MHz/50MHz.
Default: Not overclocked.
.TP
.BI "Option ""DacSpeed"" """ MHz """
Run the memory at a higher clock. Useful on some cards with display glitches
at higher resolutions. But adds the risk to damage the hardware. Use with 
caution.
.TP
.BI "Option ""FramebufferWC"" """ boolean """
If writecombine is disabled in BIOS, and you add this option in configuration
file, then the driver will try to request writecombined access to the 
framebuffer. This can drastically increase the performance on unaccelerated
server. Requires that "MTRR"-support is compiled into the OS-kernel.
Default: Disabled for V1000, enabled for V2100/V2200.
.TP
.BI "Option ""NoDDC"" """ boolean """
Disable probing of DDC-information from your monitor. This information is not
used yet and is only there for informational purposes. This might change
before final XFree86 4.0 release. Safe to disable if you experience problems
during startup of X-server.
Default: Probe DDC.
.TP
.BI "Option ""ShadowFB"" """ boolean """
If this option is enabled, the driver will cause the CPU to do each drawing
operation first into a shadow frame buffer in system virtual memory and then
copy the result into video memory. If this option is not active, the CPU will
draw directly into video memory.  Enabling this option is beneficial for those
systems where reading from video memory is, on average, slower than the
corresponding read/modify/write operation in system virtual memory.  This is 
normally the case for PCI or AGP adapters, and, so, this option is enabled by
default unless acceleration is enabled.
Default: Enabled unless acceleration is used.
.TP
.BI "Option ""Rotate"" ""CW""
.TP
.BI "Option ""Rotate"" ""CCW""
Rotate the display clockwise or counterclockwise.  This mode is unaccelerated.
Default: no rotation.
.TP
.SH "Notes"
For the moment the driver defaults to not request write-combine for any chipset
as there has been indications of problems with it. Use
.B "Option ""MTRR"""
to let the driver request write-combining of memory access on the videoboard.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1)
.SH AUTHORS
Authors include: Marc Langenbach, Dejan Ilic
