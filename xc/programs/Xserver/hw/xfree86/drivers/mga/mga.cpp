.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.cpp,v 1.13 2000/03/07 01:37:49 dawes Exp $ 
.TH MGA __drivermansuffix__ "Version 4.0"  "XFree86"
.SH NAME
mga \- Matrox video driver
.SH SYNOPSIS
.B "Section ""Device"""
.br
.BI "  Identifier """  devname """"
.br
.B  "  Driver ""mga"""
.br
\ \ ...
.br
.B EndSection
.SH DESCRIPTION
.B mga 
is an XFree86 driver for Matrox video cards.  The driver is fully
accelerated, and provides support for the following framebuffer depths:
8, 15, 16, 24, and an 8+24 overlay mode.  All
visual types are supported for depth 8, and both TrueColor and DirectColor
visuals are supported for the other depths except 8+24 mode which supports
PseudoColor, GrayScale and TrueColor.  Multi-head configurations
are supported.
.SH SUPPORTED HARDWARE
The
.B mga
driver supports PCI and AGP video cards based on the following Matrox chips:
.TP 12
.B MGA2064W
Millennium (original)
.TP 12
.B MGA1064SG
Mystique
.TP 12
.B MGA2164W
Millennium II
.TP 12
.B G100
.TP 12
.B G200
Millennium G200 and Mystique G200
.TP 12
.B G400
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
"mga2064w", "mga1064sg", "mga2164w", "mga2164w agp", "mgag100", "mgag200",
"mgag200 pci" "mgag400".
.RE
.PP
The driver will auto-detect the amount of video memory present for all
chips except the Millennium II.  In the Millennium II case it defaults
to 4096\ kBytes.  When using a Millennium II, the actual amount of video
memory should be specified with a
.B VideoRam
entry in the config file
.B """Device"""
section.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option ""ColorKey"" """ integer """
Set the colormap index used for the transparency key for the depth 8 plane
when operating in 8+24 overlay mode.  The value must be in the range
2\-255.  Default: 255.
.TP
.BI "Option ""HWCursor"" """ boolean """
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option ""MGASDRAM"" """ boolean """
Specify whether G100 and G200 cards have SDRAM.  The driver attempts to
auto-detect this based on the card's PCI subsystem ID.  This option may
be used to override that auto-detection.  The mga driver is not able to 
auto-detect the prescence SDRAM on secondary heads in multihead configurations.
Default: auto-detected.
.TP
.BI "Option ""NoAccel"" """ boolean """
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option ""OverclockMem""
Set clocks to values used by some commercial X-Servers (G100, G200 and G400
only).  Default: off.
.TP
.BI "Option ""Overlay""
Enable 8+24 overlay mode.  Only appropriate for depth 24.
.RB ( Note: 
the G100 is unaccelerated in the 8+24 overlay mode due to a missing 
hardware feature) Default: off.
.TP
.BI "Option ""PciRetry"" """ boolean """
Enable or disable PCI retries.  Default: off.
.TP
.BI "Option ""Rotate"" ""CW""
.TP
.BI "Option ""Rotate"" ""CCW""
Rotate the display clockwise or counterclockwise.  This mode is unaccelerated.
Default: no rotation.
.TP
.BI "Option ""ShadowFB"" """ boolean """
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: off.
.TP
.BI "Option ""SyncOnGreen"" """ boolean """
Enable or disable combining the sync signals with the green signal.
Default: off.
.TP
.BI "Option ""UseFBDev"" """ boolean """
Enable or disable use of on OS-specific fb interface (and is not supported
on all OSs).  See fbdevhw(__drivermansuffix__) for further information.
Default: off.
.TP
.BI "Option ""VideoKey"" """ integer """
This sets the default pixel value for the YUV video overlay key.
Default: undefined.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(1)
.SH AUTHORS
Authors include: Radoslaw Kapitan, Mark Vojkovich, and also David Dawes, Guy
Desbief, Dirk Hohndel, Doug Merritt, Andrew E. Mileski, Andrew van der Stock,
Leonard N. Zubkoff.
