#ifndef MGA_IOCTL_H
#define MGA_IOCTL_H

#include "mgalib.h"

GLbitfield mgaClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		     GLint cx, GLint cy, GLint cw, GLint ch ); 


void mgaSwapBuffers( mgaContextPtr mmesa ); 

void mgaFlushVertices( mgaContextPtr mmesa ); 
void mgaFlushVerticesLocked( mgaContextPtr mmesa );

/* upload texture
 */

void mgaDDFinish( GLcontext *ctx );

void mgaDDInitIoctlFuncs( GLcontext *ctx );
 

#endif
