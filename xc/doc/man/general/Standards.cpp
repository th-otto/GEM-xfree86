.\" $TOG: Standards.cpp /main/26 1997/10/13 14:20:48 kaleb $
.\" Copyright (c) 1993, 1994, 1996  X Consortium
.\" 
.\" Permission is hereby granted, free of charge, to any person obtaining
.\" a copy of this software and associated documentation files (the
.\" "Software"), to deal in the Software without restriction, including
.\" without limitation the rights to use, copy, modify, merge, publish,
.\" distribute, sublicense, and/or sell copies of the Software, and to
.\" permit persons to whom the Software is furnished to do so, subject to
.\" the following conditions:
.\" 
.\" The above copyright notice and this permission notice shall be included
.\" in all copies or substantial portions of the Software.
.\" 
.\" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
.\" OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
.\" MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
.\" IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
.\" OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
.\" ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
.\" OTHER DEALINGS IN THE SOFTWARE.
.\" 
.\" Except as contained in this notice, the name of the X Consortium shall
.\" not be used in advertising or otherwise to promote the sale, use or
.\" other dealings in this Software without prior written authorization
.\" from the X Consortium.
.\"
.TH XSTANDARDS __miscmansuffix__ "Release 6.3" "X Version 11"
.SH NAME
XStandards \- X Consortium Standards
.SH SYNOPSIS
The major goal of the X Consortium was to promote cooperation within the
computer industry in the creation of standard software interfaces at
all layers
in the X Window System environment.  The status of various standards, and
the software in the X11R6.3 distribution, is explained below.
.SH STANDARDS
The following documents are X Consortium standards:
.nf

X Window System Protocol
X Version 11, Release 6.3
Robert W. Scheifler

Xlib \- C Language X Interface
X Version 11, Release 6.3
James Gettys, Robert W. Scheifler, Ron Newman

X Toolkit Intrinsics \- C Language Interface
X Version 11, Release 6.3
Joel McCormack, Paul Asente, Ralph R. Swick, Donna Converse

Bitmap Distribution Format
Version 2.1
X Version 11, Release 6.3

Inter-Client Communication Conventions Manual
Version 2.0
X Version 11, Release 6.3
David Rosenthal, Stuart W. Marks

Compound Text Encoding
Version 1.1
X Version 11, Release 6.3
Robert W. Scheifler

X Logical Font Description Conventions
Version 1.5
X Version 11, Release 6.3
Jim Flowers, Stephen Gildea

X Display Manager Control Protocol
Version 1.0
X Version 11, Release 6.3
Keith Packard

X11 Nonrectangular Window Shape Extension
Version 1.0
X Version 11, Release 6.3
Keith Packard

X11 Input Extension Protocol Specification
Version 1.0
X Version 11, Release 6.3
George Sachs, Mark Patrick

X11 Input Extension Library Specification
X Version 11, Release 6.3
Mark Patrick, George Sachs

The X Font Service Protocol
Version 2.0
X Version 11, Release 6.3
Jim Fulton

PEX Protocol Specification
Version 5.1
Cheryl Huntington (architect), Paula Womack (editor)

PEXlib Specification and C Language Binding
Version 5.1
Jeff Stevenson

Inter-Client Exchange (ICE) Protocol
Version 1.0
X Version 11, Release 6.3
Robert Scheifler, Jordan Brown

Inter-Client Exchange (ICE) Library
Version 1.0
X Version 11, Release 6.3
Ralph Mor

X Session Management Protocol
Version 1.0
X Version 11, Release 6.3
Mike Wexler

X Session Management Library
Version 1.0
X Version 11, Release 6.3
Ralph Mor

The Input Method Protocol
Version 1.0
X Version 11, Release 6.3
Masahiko Narita, Hideki Hiura

X Synchronization Extension
Version 3.0
X Version 11, Release 6.3
Tim Glauert, Dave Carver, Jim Gettys, David P. Wiggins

X Image Extension, Protocol Reference Manual
Version 5.0
X Version 11, Release 6.3
Bob Shelley

XTEST Extension
Version 2.2
Kieron Drake

Big Requests Extension
Version 2.0
X Version 11, Release 6.3
Bob Scheifler

XC-MISC Extension
Version 1.1
X Version 11, Release 6.3
Bob Scheifler, Dave Wiggins

Double Buffer Extension
Version 1.0
Ian Elliott, David P. Wiggins

Record Extension Protocol
Version 1.13
Martha Zimet, Stephen Gildea

Record Extension Library
Version 1.13
Martha Zimet, Stephen Gildea

X Keyboard Extension Protocol
X Version 11, Release 6.3
Erik Fortune

X Keyboard Extension Library
X Version 11, Release 6.3
Amber J. Benson, Gary Aitken, Erik Fortune, Donna Converse,
George Sachs, and Will Walker

X Print Extension Protocol
X Version 11, Release 6.3

X Print Extension Library
X Version 11, Release 6.3

X Application Group Extension Protocol and Library
Version 1.0
X Version 11, Release 6.3
Kaleb Keithley

X Security Extension Protocol and Library
Version 4.0
X Version 11, Release 6.3
Dave Wiggins

X Proxy Manager Protocol
X Version 11, Release 6.3
Ralph Swick

LBX Extension Protocol and Library
X Version 11, Release 6.3
Keith Packard, Dave Lemke, Donna Converse, Ralph Mor, Ray Tice

Remote Execution MIME Type
Version 1.0
X Version 11, Release 6.3
Arnaud Le Hors
.fi
.SH "DRAFT STANDARDS"
The following documents are currently draft standards of the X Consortium.
.nf

X Image Extension Library
Public Review Draft
Gary Rogers

PEX Protocol Specification
Version 5.2
Jeff Stevenson (architect), Jane Sczechowski (editor)

PEXlib Specification and C Language Binding
Version 5.2
Karl Schultz
.fi

.SH "INCLUDE FILES"
The following include files are part of the Xlib standard.
.PP
.nf
<X11/cursorfont.h>
<X11/keysym.h>
<X11/keysymdef.h>
<X11/X.h>
<X11/Xatom.h>
<X11/Xcms.h>
<X11/Xlib.h>
<X11/Xlibint.h>
<X11/Xproto.h>
<X11/Xprotostr.h>
<X11/Xresource.h>
<X11/Xutil.h>
<X11/X10.h>
.fi
.PP
The following include files are part of the X Toolkit Intrinsics standard.
.PP
.nf
<X11/Composite.h>
<X11/CompositeP.h>
<X11/Constraint.h>
<X11/ConstrainP.h>
<X11/Core.h>
<X11/CoreP.h>
<X11/Intrinsic.h>
<X11/IntrinsicP.h>
<X11/Object.h>
<X11/ObjectP.h>
<X11/RectObj.h>
<X11/RectObjP.h>
<X11/Shell.h>
<X11/ShellP.h>
<X11/StringDefs.h>
<X11/Vendor.h>
<X11/VendorP.h>
.fi
.PP
The following include file is part of the
Nonrectangular Window Shape Extension standard.
.PP
.nf
<X11/extensions/shape.h>
.fi
.PP
The following include files are part of the X Input Extension standard.
.PP
.nf
<X11/extensions/XI.h>
<X11/extensions/XInput.h>
<X11/extensions/XIproto.h>
.fi
.PP
The following include files are part of the PEXlib standard.
.PP
.nf
<X11/PEX5/PEX.h>
<X11/PEX5/PEXlib.h>
<X11/PEX5/PEXlibint.h>
<X11/PEX5/PEXproto.h>
<X11/PEX5/PEXprotost.h>
.fi
.PP
The following include files are part of the ICElib standard.
.PP
.nf
<X11/ICE/ICE.h>
<X11/ICE/ICEconn.h>
<X11/ICE/ICElib.h>
<X11/ICE/ICEmsg.h>
<X11/ICE/ICEproto.h>
<X11/ICE/ICEutil.h>
.fi
.PP
The following include files are part of the SMlib standard.
.PP
.nf
<X11/SM/SM.h>
<X11/SM/SMlib.h>
<X11/SM/SMproto.h>
.fi
.PP
The following include file is part of the Synchronization standard.
.PP
.nf
<X11/extensions/sync.h>
.fi
.PP
The following include files are part of the XIElib draft standard.
.PP
.nf
<X11/extensions/XIE.h>
<X11/extensions/XIElib.h>
<X11/extensions/XIEproto.h>
<X11/extensions/XIEprotost.h>
.fi
.PP
The following include file is part of the XTEST standard.
.PP
.nf
<X11/extensions/XTest.h>
.fi
.PP
The following include file is part of the Double Buffer Extension standard.
.PP
.nf
<X11/extensions/Xdbe.h>
.fi
.PP
The following include file is part of the Record Library standard.
.PP
.nf
<X11/extensions/record.h>
.fi
.PP
The following include files are part of the X Keyboard Extension Library
standard.
.PP
.nf
\" some subset of...
<X11/XKBlib.h>
<X11/extensions/XKB.h>
<X11/extensions/XKBproto.h>
<X11/extensions/XKBstr.h>
<X11/extensions/XKBgeom.h>
.fi
.PP
The following include files are part of the X Print Extension Library
standard.
.PP
.nf
<X11/extensions/Print.h>
<X11/extensions/Printstr.h>
.fi
.PP
The following include files are part of the X Application Group Extension
Library standard.
.PP
.nf
<X11/extensions/Xag.h>
<X11/extensions/Xagstr.h>
.fi
.PP
The following include files are part of the X Security Extension Library
standard.
.PP
.nf
<X11/extensions/security.h>
<X11/extensions/securstr.h>
.fi
.PP
The following include files are part of the LBX Extension library standard.
.PP
.nf
\" some subset of...
<X11/extensions/XLbx.h>
<X11/extensions/lbxbuf.h>
<X11/extensions/lbxbufstr.h>
<X11/extensions/lbxdeltastr.h>
<X11/extensions/lbximage.h>
<X11/extensions/lbxopts.h>
<X11/extensions/lbxstr.h>
<X11/extensions/lbxzlib.h>
.fi

.SH "NON STANDARDS"
The X11R6.3 distribution contains \fIsample\fP implementations, not
\fIreference\fP implementations.  Although much of the code is believed
to be correct, the code should be assumed to be in error wherever it
conflicts with the specification.
.PP
The only X Consortium standards are the ones listed above.
No other documents, include files, or software in X11R6.3 carry special
status within the X Consortium.  For example, none of the following
are standards:
internal interfaces of the sample server;
the MIT-SHM extension;
the Athena Widget Set;
the Xmu library;
the Xau library;
the RGB database;
the X Locale database;
the fonts distributed with X11R6.3;
the applications distributed with X11R6.3;
the include files <X11/XWDFile.h>, <X11/Xfuncproto.h>, <X11/Xfuncs.h>,
<X11/Xosdefs.h>, <X11/Xos.h>, <X11/Xos_r.h>, <X11/Xwinsock.h>, and 
<X11/Xthreads.h>;
the bitmap files in <X11/bitmaps>.
.PP
The Multi-Buffering extension was a draft standard of the
X Consortium but has been superseded by DBE as a standard.

.SH "X REGISTRY"
The X Consortium maintained a registry of certain X-related items, to
aid in avoiding conflicts and to aid in sharing of such items.
.PP
The registry is published as part of the X Consortium software
release.
The latest version may also be available by sending a message to
xstuff@x.org.  The message can have a subject line and no body, or a
single-line body and no subject, in either case the line looking like:
.nf
	send docs registry
.fi
The X Registry and the names in it are not X Consortium standards.
