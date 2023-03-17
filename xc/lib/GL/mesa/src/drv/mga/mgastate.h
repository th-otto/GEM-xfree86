#ifndef _MGA_STATE_H
#define _MGA_STATE_H


extern void mgaDDInitStateFuncs(GLcontext *ctx);
extern void mgaDDUpdateHwState( GLcontext *ctx );
extern void mgaDDUpdateState( GLcontext *ctx );
extern void mgaDDReducedPrimitiveChange( GLcontext *ctx, GLenum prim );

/* reprograms the current registers without updating them, used to
reset state after a dma buffer overflow */
void mgaUpdateRegs( GLuint regs );


#endif
