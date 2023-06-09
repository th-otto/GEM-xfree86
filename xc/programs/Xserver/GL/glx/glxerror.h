#ifndef _GLX_error_h_
#define _GLX_error_h_

/* $XFree86: xc/programs/Xserver/GL/glx/glxerror.h,v 1.2 1999/06/14 07:31:26 dawes Exp $ */
/*
** The contents of this file are subject to the GLX Public License Version 1.0
** (the "License"). You may not use this file except in compliance with the
** License. You may obtain a copy of the License at Silicon Graphics, Inc.,
** attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA 94043
** or at http://www.sgi.com/software/opensource/glx/license.html.
**
** Software distributed under the License is distributed on an "AS IS"
** basis. ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY
** IMPLIED WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
** PURPOSE OR OF NON- INFRINGEMENT. See the License for the specific
** language governing rights and limitations under the License.
**
** The Original Software is GLX version 1.2 source code, released February,
** 1999. The developer of the Original Software is Silicon Graphics, Inc.
** Those portions of the Subject Software created by Silicon Graphics, Inc.
** are Copyright (c) 1991-9 Silicon Graphics, Inc. All Rights Reserved.
**
** $SGI$
*/

/*
** Error codes.  These have the extension error base added to them
** when the extension initializes.
*/
extern int __glXBadContext;
extern int __glXBadContextState;
extern int __glXBadDrawable;
extern int __glXBadPixmap;
extern int __glXBadCurrentWindow;
extern int __glXBadContextTag;
extern int __glXBadRenderRequest;
extern int __glXBadLargeRequest;
extern int __glXUnsupportedPrivateRequest;

#endif
