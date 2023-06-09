/* $XFree86: xc/lib/GL/glx/render2.c,v 1.2 1999/06/14 07:23:39 dawes Exp $ */
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

#include "packrender.h"

/*
** This file contains routines that might need to be transported as
** GLXRender or GLXRenderLarge commands, and these commands don't
** use the pixel header.  See renderpix.c for those routines.
*/

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLX_DECLARE_VARIABLES();

    compsize = __glCallLists_size(n,type);
    cmdlen = __GLX_PAD(12 + compsize);
    __GLX_LOAD_VARIABLES();
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_CallLists,cmdlen);
	__GLX_PUT_LONG(4,n);
	__GLX_PUT_LONG(8,type);
	__GLX_PUT_CHAR_ARRAY(12,lists,compsize);
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_CallLists,cmdlen+4);
	__GLX_PUT_LONG(8,n);
	__GLX_PUT_LONG(12,type);
	__glXSendLargeCommand(gc, pc, 16, lists, compsize);
    }
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride,
	     GLint order, const GLdouble *pnts)
{
    __GLX_DECLARE_VARIABLES();
    GLint k;

    __GLX_LOAD_VARIABLES();
    k = __glEvalComputeK(target);
    if (k == 0) {
	__glXSetError(gc, GL_INVALID_ENUM);
	return;
    } else if (stride < k || order <= 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = k * order * __GLX_SIZE_FLOAT64;
    cmdlen = 28+compsize;
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_Map1d,cmdlen);
	__GLX_PUT_DOUBLE(4,u1);
	__GLX_PUT_DOUBLE(12,u2);
	__GLX_PUT_LONG(20,target);
	__GLX_PUT_LONG(24,order);
	/*
	** NOTE: the doubles that follow are not aligned because of 3
	** longs preceeding
	*/
	__glFillMap1d(k, order, stride, pnts, (pc+28));
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_Map1d,cmdlen+4);
	__GLX_PUT_DOUBLE(8,u1);
	__GLX_PUT_DOUBLE(16,u2);
	__GLX_PUT_LONG(24,target);
	__GLX_PUT_LONG(28,order);

	/*
	** NOTE: the doubles that follow are not aligned because of 3
	** longs preceeding
	*/
	if (stride != k) {
	    GLubyte *buf;

	    buf = (GLubyte *) Xmalloc(compsize);
	    if (!buf) {
		__glXSetError(gc, GL_OUT_OF_MEMORY);
		return;
	    }
	    __glFillMap1d(k, order, stride, pnts, buf);
	    __glXSendLargeCommand(gc, pc, 32, buf, compsize);
	    Xfree((char*) buf);
	} else {
	    /* Data is already packed.  Just send it out */
	    __glXSendLargeCommand(gc, pc, 32, pnts, compsize);
	}
    }
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride,
	     GLint order, const GLfloat *pnts)
{
    __GLX_DECLARE_VARIABLES();
    GLint k;

    __GLX_LOAD_VARIABLES();
    k = __glEvalComputeK(target);
    if (k == 0) {
	__glXSetError(gc, GL_INVALID_ENUM);
	return;
    } else if (stride < k || order <= 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = k * order * __GLX_SIZE_FLOAT32;
    cmdlen = 20+compsize;
    if (!gc->currentDpy) return;

    /*
    ** The order that arguments are packed is different from the order
    ** for glMap1d.
    */
    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_Map1f,cmdlen);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_FLOAT(8,u1);
	__GLX_PUT_FLOAT(12,u2);
	__GLX_PUT_LONG(16,order);
	__glFillMap1f(k, order, stride, pnts, (GLubyte*) (pc+20));
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_Map1f,cmdlen+4);
	__GLX_PUT_LONG(8,target);
	__GLX_PUT_FLOAT(12,u1);
	__GLX_PUT_FLOAT(16,u2);
	__GLX_PUT_LONG(20,order);

	if (stride != k) {
	    GLubyte *buf;

	    buf = (GLubyte *) Xmalloc(compsize);
	    if (!buf) {
		__glXSetError(gc, GL_OUT_OF_MEMORY);
		return;
	    }
	    __glFillMap1f(k, order, stride, pnts, buf);
	    __glXSendLargeCommand(gc, pc, 24, buf, compsize);
	    Xfree((char*) buf);
	} else {
	    /* Data is already packed.  Just send it out */
	    __glXSendLargeCommand(gc, pc, 24, pnts, compsize);
	}
    }
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustr, GLint uord,
	     GLdouble v1, GLdouble v2, GLint vstr, GLint vord,
	     const GLdouble *pnts)
{
    __GLX_DECLARE_VARIABLES();
    GLint k;

    __GLX_LOAD_VARIABLES();
    k = __glEvalComputeK(target);
    if (k == 0) {
	__glXSetError(gc, GL_INVALID_ENUM);
	return;
    } else if (vstr < k || ustr < k || vord <= 0 || uord <= 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = k * uord * vord * __GLX_SIZE_FLOAT64;
    cmdlen = 48+compsize; 
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_Map2d,cmdlen);
	__GLX_PUT_DOUBLE(4,u1);
	__GLX_PUT_DOUBLE(12,u2);
	__GLX_PUT_DOUBLE(20,v1);
	__GLX_PUT_DOUBLE(28,v2);
	__GLX_PUT_LONG(36,target);
	__GLX_PUT_LONG(40,uord);
	__GLX_PUT_LONG(44,vord);
	/*
	** Pack into a u-major ordering.
	** NOTE: the doubles that follow are not aligned because of 5
	** longs preceeding
	*/
	__glFillMap2d(k, uord, vord, ustr, vstr, pnts, (GLdouble*) (pc+48));
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_Map2d,cmdlen+4);
	__GLX_PUT_DOUBLE(8,u1);
	__GLX_PUT_DOUBLE(16,u2);
	__GLX_PUT_DOUBLE(24,v1);
	__GLX_PUT_DOUBLE(32,v2);
	__GLX_PUT_LONG(40,target);
	__GLX_PUT_LONG(44,uord);
	__GLX_PUT_LONG(48,vord);

	/*
	** NOTE: the doubles that follow are not aligned because of 5
	** longs preceeding
	*/
	if ((vstr != k) || (ustr != k*vord)) {
	    GLdouble *buf;

	    buf = (GLdouble *) Xmalloc(compsize);
	    if (!buf) {
		__glXSetError(gc, GL_OUT_OF_MEMORY);
		return;
	    }
	    /*
	    ** Pack into a u-major ordering.
	    */
	    __glFillMap2d(k, uord, vord, ustr, vstr, pnts, buf);
	    __glXSendLargeCommand(gc, pc, 52, buf, compsize);
	    Xfree((char*) buf);
	} else {
	    /* Data is already packed.  Just send it out */
	    __glXSendLargeCommand(gc, pc, 52, pnts, compsize);
	}
    }
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustr, GLint uord,
	     GLfloat v1, GLfloat v2, GLint vstr, GLint vord,
	     const GLfloat *pnts)
{
    __GLX_DECLARE_VARIABLES();
    GLint k;

    __GLX_LOAD_VARIABLES();
    k = __glEvalComputeK(target);
    if (k == 0) {
	__glXSetError(gc, GL_INVALID_ENUM);
	return;
    } else if (vstr < k || ustr < k || vord <= 0 || uord <= 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = k * uord * vord * __GLX_SIZE_FLOAT32;
    cmdlen = 32+compsize; 
    if (!gc->currentDpy) return;

    /*
    ** The order that arguments are packed is different from the order
    ** for glMap2d.
    */
    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_Map2f,cmdlen);
	__GLX_PUT_LONG(4,target);
	__GLX_PUT_FLOAT(8,u1);
	__GLX_PUT_FLOAT(12,u2);
	__GLX_PUT_LONG(16,uord);
	__GLX_PUT_FLOAT(20,v1);
	__GLX_PUT_FLOAT(24,v2);
	__GLX_PUT_LONG(28,vord);
	/*
	** Pack into a u-major ordering.
	*/
	__glFillMap2f(k, uord, vord, ustr, vstr, pnts, (GLfloat*) (pc+32));
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_Map2f,cmdlen+4);
	__GLX_PUT_LONG(8,target);
	__GLX_PUT_FLOAT(12,u1);
	__GLX_PUT_FLOAT(16,u2);
	__GLX_PUT_LONG(20,uord);
	__GLX_PUT_FLOAT(24,v1);
	__GLX_PUT_FLOAT(28,v2);
	__GLX_PUT_LONG(32,vord);

	if ((vstr != k) || (ustr != k*vord)) {
	    GLfloat *buf;

	    buf = (GLfloat *) Xmalloc(compsize);
	    if (!buf) {
		__glXSetError(gc, GL_OUT_OF_MEMORY);
		return;
	    }
	    /*
	    ** Pack into a u-major ordering.
	    */
	    __glFillMap2f(k, uord, vord, ustr, vstr, pnts, buf);
	    __glXSendLargeCommand(gc, pc, 36, buf, compsize);
	    Xfree((char*) buf);
	} else {
	    /* Data is already packed.  Just send it out */
	    __glXSendLargeCommand(gc, pc, 36, pnts, compsize);
	}
    }
}

void glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (mapsize < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = mapsize * __GLX_SIZE_FLOAT32;
    cmdlen = 12+compsize;
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_PixelMapfv,cmdlen);
	__GLX_PUT_LONG(4,map);
	__GLX_PUT_LONG(8,mapsize);
	__GLX_PUT_FLOAT_ARRAY(12,values,mapsize);
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_PixelMapfv,cmdlen+4);
	__GLX_PUT_LONG(8,map);
	__GLX_PUT_LONG(12,mapsize);
	__glXSendLargeCommand(gc, pc, 16, values, compsize);
    }
}

void glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (mapsize < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = mapsize * __GLX_SIZE_CARD32;
    cmdlen = 12+compsize;
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_PixelMapuiv,cmdlen);
	__GLX_PUT_LONG(4,map);
	__GLX_PUT_LONG(8,mapsize);
	__GLX_PUT_LONG_ARRAY(12,values,mapsize);
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_PixelMapuiv,cmdlen+4);
	__GLX_PUT_LONG(8,map);
	__GLX_PUT_LONG(12,mapsize);
	__glXSendLargeCommand(gc, pc, 16, values, compsize);
    }
}

void glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (mapsize < 0) {
	__glXSetError(gc, GL_INVALID_VALUE);
	return;
    }
    compsize = mapsize * __GLX_SIZE_CARD16;
    cmdlen = __GLX_PAD(12 + compsize);
    if (!gc->currentDpy) return;

    if (cmdlen <= gc->maxSmallRenderCommandSize) {
	/* Use GLXRender protocol to send small command */
	__GLX_BEGIN_VARIABLE(X_GLrop_PixelMapusv,cmdlen);
	__GLX_PUT_LONG(4,map);
	__GLX_PUT_LONG(8,mapsize);
	__GLX_PUT_SHORT_ARRAY(12,values,mapsize);
	__GLX_END(cmdlen);
    } else {
	/* Use GLXRenderLarge protocol to send command */
	__GLX_BEGIN_VARIABLE_LARGE(X_GLrop_PixelMapusv,cmdlen+4);
	__GLX_PUT_LONG(8,map);
	__GLX_PUT_LONG(12,mapsize);
	__glXSendLargeCommand(gc, pc, 16, values, compsize);
    }
}

void glEnable(GLenum cap)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (!gc->currentDpy) return;

    switch (cap) {
      case GL_VERTEX_ARRAY:
	  gc->state.vertArray.vertexEnable = GL_TRUE;
	  return;
      case GL_NORMAL_ARRAY:
	  gc->state.vertArray.normalEnable = GL_TRUE;
	  return;
      case GL_COLOR_ARRAY:
	  gc->state.vertArray.colorEnable = GL_TRUE;
	  return;
      case GL_INDEX_ARRAY:
	  gc->state.vertArray.indexEnable = GL_TRUE;
	  return;
      case GL_TEXTURE_COORD_ARRAY:
	  gc->state.vertArray.texCoordEnable = GL_TRUE;
	  return;
      case GL_EDGE_FLAG_ARRAY:
	  gc->state.vertArray.edgeFlagEnable = GL_TRUE;
	  return;
    }
    __GLX_BEGIN(X_GLrop_Enable,8);
    __GLX_PUT_LONG(4,cap);
    __GLX_END(8);
}

void glDisable(GLenum cap)
{
    __GLX_DECLARE_VARIABLES();

    __GLX_LOAD_VARIABLES();
    if (!gc->currentDpy) return;

    switch (cap) {
      case GL_VERTEX_ARRAY:
	  gc->state.vertArray.vertexEnable = GL_FALSE;
	  return;
      case GL_NORMAL_ARRAY:
	  gc->state.vertArray.normalEnable = GL_FALSE;
	  return;
      case GL_COLOR_ARRAY:
	  gc->state.vertArray.colorEnable = GL_FALSE;
	  return;
      case GL_INDEX_ARRAY:
	  gc->state.vertArray.indexEnable = GL_FALSE;
	  return;
      case GL_TEXTURE_COORD_ARRAY:
	  gc->state.vertArray.texCoordEnable = GL_FALSE;
	  return;
      case GL_EDGE_FLAG_ARRAY:
	  gc->state.vertArray.edgeFlagEnable = GL_FALSE;
	  return;
    }
    __GLX_BEGIN(X_GLrop_Disable,8);
    __GLX_PUT_LONG(4,cap);
    __GLX_END(8);
}
