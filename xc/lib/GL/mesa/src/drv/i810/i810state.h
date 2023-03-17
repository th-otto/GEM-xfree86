#ifndef _I810_STATE_H
#define _I810_STATE_H


extern void i810DDUpdateHwState( GLcontext *ctx );
extern void i810DDUpdateState( GLcontext *ctx );
extern void i810DDInitState( i810ContextPtr imesa );
extern void i810DDInitStateFuncs( GLcontext *ctx );

extern void i810DDPrintState( const char *msg, GLuint state );


#endif
