#ifndef __GLX_glx_h__
#define __GLX_glx_h__

/* $XFree86: xc/include/GL/glx.h,v 1.5 2000/03/02 16:07:29 martin Exp $ */
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <GL/gl.h>
#include <GL/glxtokens.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** GLX resources.
*/
typedef XID GLXContextID;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
/* GLX 1.3 */
typedef XID GLXFBConfigID;
typedef XID GLXPfuffer;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
typedef XID GLXFBConfig;


/*
** GLXContext is a pointer to opaque data.
**/
typedef struct __GLXcontextRec *GLXContext;


/************************************************************************/

extern XVisualInfo* glXChooseVisual (Display *dpy, int screen, int *attribList);
extern void glXCopyContext (Display *dpy, GLXContext src, GLXContext dst, unsigned long mask);
extern GLXContext glXCreateContext (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
extern GLXPixmap glXCreateGLXPixmap (Display *dpy, XVisualInfo *vis, Pixmap pixmap);
extern void glXDestroyContext (Display *dpy, GLXContext ctx);
extern void glXDestroyGLXPixmap (Display *dpy, GLXPixmap pix);
extern int glXGetConfig (Display *dpy, XVisualInfo *vis, int attrib, int *value);
extern GLXContext glXGetCurrentContext (void);
extern GLXDrawable glXGetCurrentDrawable (void);
extern Bool glXIsDirect (Display *dpy, GLXContext ctx);
extern Bool glXMakeCurrent (Display *dpy, GLXDrawable drawable, GLXContext ctx);
extern Bool glXQueryExtension (Display *dpy, int *errorBase, int *eventBase);
extern Bool glXQueryVersion (Display *dpy, int *major, int *minor);
extern void glXSwapBuffers (Display *dpy, GLXDrawable drawable);
extern void glXUseXFont (Font font, int first, int count, int listBase);
extern void glXWaitGL (void);
extern void glXWaitX (void);
extern const char * glXGetClientString (Display *dpy, int name );
extern const char * glXQueryServerString (Display *dpy, int screen, int name );
extern const char * glXQueryExtensionsString (Display *dpy, int screen );

/* GLX 1.3 */
extern GLXFBConfig glXChooseFBConfig (Display *dpy, int screen, const int *attribList, int *nitems);
extern int glXGetFBConfigAttrib (Display *dpy, GLXFBConfig config, int attribute, int *value);
extern XVisualInfo * glXGetVisualFromFBConfig (Display *dpy, GLXFBConfig config);
extern GLXWindow glXCreateWindow (Display *dpy, GLXFBConfig config, Window win, const int *attribList);
extern void glXDestroyWindow (Display *dpy, GLXWindow window);
extern GLXPixmap glXCreatePixmap (Display *dpy, GLXFBConfig config,Pixmap pixmap, const int *attribList);
extern void glXDestroyPixmap (Display *dpy, GLXPixmap pixmap);
extern GLXPbuffer glXCreatePbuffer (Display *dpy, GLXFBConfig config, const int *attribList);
extern void glXDestroyPbuffer (Display *dpy, GLXPbuffer pbuf);
extern void glXQueryDrawable (Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
extern GLXContext glXCreateNewContext (Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct);
extern Bool glXMakeContextCurrent (Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
extern GLXDrawable glXGetCurrentReadDrawable (void);
extern int glXQueryContext (Display *dpy, GLXContext ctx, int attribute, int *value);
extern void glXSelectEvent (Display *dpy, GLXDrawable drawable, unsigned long mask);
extern void glXGetSelectedEvent (Display *dpy, GLXDrawable drawable, unsigned long *mask);

/* Extensions */
extern Display * glXGetCurrentDisplay (void);
extern GLXContextID glXGetContextIDEXT (const GLXContext ctx);
extern GLXDrawable glXGetCurrentDrawableEXT (void);
extern GLXContext glXImportContextEXT (Display *dpy, GLXContextID contextID);
extern void glXFreeContextEXT (Display *dpy, GLXContext ctx);
extern int glXQueryContextInfoEXT (Display *dpy, GLXContext ctx, int attribute, int *value);
extern void (*glXGetProcAddressARB(const GLubyte *procName))();



#ifdef __cplusplus
}
#endif

#endif /* !__GLX_glx_h__ */
