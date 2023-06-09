/* $XFree86: xc/programs/Xserver/GL/glx/render2swap.c,v 1.4 1999/07/18 08:34:24 dawes Exp $ */
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

/* #define NEED_REPLIES */
#include "glxserver.h"
#include "unpack.h"
#include "g_disptab.h"

extern GLint __glEvalComputeK(GLenum target);

void __glXDispSwap_Map1f(GLbyte *pc)
{
    GLint order, k;
    GLfloat u1, u2, *points;
    GLenum target;
    GLint compsize;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(pc + 0);
    __GLX_SWAP_INT(pc + 12);
    __GLX_SWAP_FLOAT(pc + 4);
    __GLX_SWAP_FLOAT(pc + 8);
    
    target = *(GLenum *)(pc + 0); 
    order = *(GLint *)(pc + 12);
    u1 = *(GLfloat *)(pc + 4);
    u2 = *(GLfloat *)(pc + 8);
    points = (GLfloat *)(pc + 16);
    k = __glEvalComputeK(target);

    if (order <= 0 || k < 0) {
	/* Erroneous command. */
	compsize = 0;
    } else {
	compsize = order * k;
    }
    __GLX_SWAP_FLOAT_ARRAY(points, compsize);

    glMap1f(target, u1, u2, k, order, points);
}

void __glXDispSwap_Map2f(GLbyte *pc)
{
    GLint uorder, vorder, ustride, vstride, k;
    GLfloat u1, u2, v1, v2, *points;
    GLenum target;
    GLint compsize;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(pc + 0);
    __GLX_SWAP_INT(pc + 12);
    __GLX_SWAP_INT(pc + 24);
    __GLX_SWAP_FLOAT(pc + 4);
    __GLX_SWAP_FLOAT(pc + 8);
    __GLX_SWAP_FLOAT(pc + 16);
    __GLX_SWAP_FLOAT(pc + 20);
    
    target = *(GLenum *)(pc + 0); 
    uorder = *(GLint *)(pc + 12);
    vorder = *(GLint *)(pc + 24);
    u1 = *(GLfloat *)(pc + 4);
    u2 = *(GLfloat *)(pc + 8);
    v1 = *(GLfloat *)(pc + 16);
    v2 = *(GLfloat *)(pc + 20);
    points = (GLfloat *)(pc + 28);

    k = __glEvalComputeK(target);
    ustride = vorder * k;
    vstride = k;

    if (vorder <= 0 || uorder <= 0 || k < 0) {
	/* Erroneous command. */
	compsize = 0;
    } else {
	compsize = uorder * vorder * k;
    }
    __GLX_SWAP_FLOAT_ARRAY(points, compsize);

    glMap2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void __glXDispSwap_Map1d(GLbyte *pc)
{
    GLint order, k, compsize;
    GLenum target;
    GLdouble u1, u2, *points;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_DOUBLE(pc + 0);
    __GLX_SWAP_DOUBLE(pc + 8);
    __GLX_SWAP_INT(pc + 16);
    __GLX_SWAP_INT(pc + 20);

    target = *(GLenum*) (pc + 16);
    order = *(GLint*) (pc + 20);
    k = __glEvalComputeK(target);
    if (order <= 0 || k < 0) {
	/* Erroneous command. */
	compsize = 0;
    } else {
	compsize = order * k;
    }
    __GLX_GET_DOUBLE(u1,pc);
    __GLX_GET_DOUBLE(u2,pc+8);
    __GLX_SWAP_DOUBLE_ARRAY(pc+24, compsize);
    pc += 24;

#ifdef __GLX_ALIGN64
    if (((unsigned long)pc) & 7) {
	/*
	** Copy the doubles up 4 bytes, trashing the command but aligning
	** the data in the process
	*/
	__GLX_MEM_COPY(pc-4, pc, compsize*8);
	points = (GLdouble*) (pc - 4);
    } else {
	points = (GLdouble*) pc;
    }
#else
    points = (GLdouble*) pc;
#endif
    glMap1d(target, u1, u2, k, order, points);
}

void __glXDispSwap_Map2d(GLbyte *pc)
{
    GLdouble u1, u2, v1, v2, *points;
    GLint uorder, vorder, ustride, vstride, k, compsize;
    GLenum target;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_DOUBLE(pc + 0);
    __GLX_SWAP_DOUBLE(pc + 8);
    __GLX_SWAP_DOUBLE(pc + 16);
    __GLX_SWAP_DOUBLE(pc + 24);
    __GLX_SWAP_INT(pc + 32);
    __GLX_SWAP_INT(pc + 36);
    __GLX_SWAP_INT(pc + 40);

    target = *(GLenum *)(pc + 32);
    uorder = *(GLint *)(pc + 36);
    vorder = *(GLint *)(pc + 40);
    k = __glEvalComputeK(target);
    if (vorder <= 0 || uorder <= 0 || k < 0) {
	/* Erroneous command. */
	compsize = 0;
    } else {
	compsize = uorder * vorder * k;
    }
    __GLX_GET_DOUBLE(u1,pc);
    __GLX_GET_DOUBLE(u2,pc+8);
    __GLX_GET_DOUBLE(v1,pc+16);
    __GLX_GET_DOUBLE(v2,pc+24);
    __GLX_SWAP_DOUBLE_ARRAY(pc+44, compsize);
    pc += 44;
    ustride = vorder * k;
    vstride = k;

#ifdef __GLX_ALIGN64
    if (((unsigned long)pc) & 7) {
	/*
	** Copy the doubles up 4 bytes, trashing the command but aligning
	** the data in the process
	*/
	__GLX_MEM_COPY(pc-4, pc, compsize*8);
	points = (GLdouble*) (pc - 4);
    } else {
	points = (GLdouble*) pc;
    }
#else
    points = (GLdouble*) pc;
#endif
    glMap2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void __glXDispSwap_CallLists(GLbyte *pc)
{
    GLenum type;
    GLsizei n;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(pc + 4);
    __GLX_SWAP_INT(pc + 0);
    type = *(GLenum *)(pc + 4);
    n = *(GLsizei *)(pc + 0);

    switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_2_BYTES:
      case GL_3_BYTES:
      case GL_4_BYTES:
	break;
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
	__GLX_SWAP_SHORT_ARRAY(pc+8, n);
	break;
      case GL_INT:
      case GL_UNSIGNED_INT:
	__GLX_SWAP_INT_ARRAY(pc+8, n);
	break;
      case GL_FLOAT:
	__GLX_SWAP_FLOAT_ARRAY(pc+8, n);
	break;
    }

    glCallLists(n, type, pc+8);
}

static void swapArray(GLint numVals, GLenum datatype,
                      GLint stride, GLint numVertexes, GLbyte *pc)
{
    int i,j;
    __GLX_DECLARE_SWAP_VARIABLES;

    switch (datatype) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
	/* don't need to swap */
	return;
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
	for (i=0; i<numVertexes; i++) {
	    GLshort *pVal = (GLshort *) pc;
	    for (j=0; j<numVals; j++) {
		__GLX_SWAP_SHORT(&pVal[j]);
	    }
	    pc += stride;
	}
	break;
      case GL_INT:
      case GL_UNSIGNED_INT:
	for (i=0; i<numVertexes; i++) {
	    GLint *pVal = (GLint *) pc;
	    for (j=0; j<numVals; j++) {
		__GLX_SWAP_INT(&pVal[j]);
	    }
	    pc += stride;
	}
	break;
      case GL_FLOAT:
	for (i=0; i<numVertexes; i++) {
	    GLfloat *pVal = (GLfloat *) pc;
	    for (j=0; j<numVals; j++) {
		__GLX_SWAP_FLOAT(&pVal[j]);
	    }
	    pc += stride;
	}
	break;
      case GL_DOUBLE:
	for (i=0; i<numVertexes; i++) {
	    GLdouble *pVal = (GLdouble *) pc;
	    for (j=0; j<numVals; j++) {
		__GLX_SWAP_DOUBLE(&pVal[j]);
	    }
	    pc += stride;
	}
	break;
      default:
	return;
    }
}

void __glXDispSwap_DrawArrays(GLbyte *pc)
{
    __GLXdispatchDrawArraysHeader *hdr = (__GLXdispatchDrawArraysHeader *)pc;
    __GLXdispatchDrawArraysComponentHeader *compHeader;
    GLint numVertexes = hdr->numVertexes;
    GLint numComponents = hdr->numComponents;
    GLenum primType = hdr->primType;
    GLint stride = 0;
    int i;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(&numVertexes);
    __GLX_SWAP_INT(&numComponents);
    __GLX_SWAP_INT(&primType);

    pc += sizeof(__GLXdispatchDrawArraysHeader);
    compHeader = (__GLXdispatchDrawArraysComponentHeader *) pc;

    /* compute stride (same for all component arrays) */
    for (i=0; i<numComponents; i++) {
	GLenum datatype = compHeader[i].datatype;
	GLint numVals = compHeader[i].numVals;
	GLenum component = compHeader[i].component;

	__GLX_SWAP_INT(&datatype);
	__GLX_SWAP_INT(&numVals);
	__GLX_SWAP_INT(&component);

        stride += __GLX_PAD(numVals * __glXTypeSize(datatype));
    }

    pc += numComponents * sizeof(__GLXdispatchDrawArraysComponentHeader);

    /* set up component arrays */
    for (i=0; i<numComponents; i++) {
	GLenum datatype = compHeader[i].datatype;
	GLint numVals = compHeader[i].numVals;
	GLenum component = compHeader[i].component;

	swapArray(numVals, datatype, stride, numVertexes, pc);

        switch (component) {
          case GL_VERTEX_ARRAY:
	    glEnableClientState(GL_VERTEX_ARRAY);
	    glVertexPointer(numVals, datatype, stride, pc);
            break;
          case GL_NORMAL_ARRAY:
	    glEnableClientState(GL_NORMAL_ARRAY);
	    glNormalPointer(datatype, stride, pc);
            break;
          case GL_COLOR_ARRAY:
	    glEnableClientState(GL_COLOR_ARRAY);
	    glColorPointer(numVals, datatype, stride, pc);
            break;
          case GL_INDEX_ARRAY:
	    glEnableClientState(GL_INDEX_ARRAY);
	    glIndexPointer(datatype, stride, pc);
            break;
          case GL_TEXTURE_COORD_ARRAY:
	    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	    glTexCoordPointer(numVals, datatype, stride, pc);
            break;
          case GL_EDGE_FLAG_ARRAY:
	    glEnableClientState(GL_EDGE_FLAG_ARRAY);
	    glEdgeFlagPointer(stride, (const GLboolean *)pc);
            break;
          default:
            break;
	}

        pc += __GLX_PAD(numVals * __glXTypeSize(datatype));
    }

    glDrawArrays(primType, 0, numVertexes);

    /* turn off anything we might have turned on */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_EDGE_FLAG_ARRAY);
}
