
#ifndef _MGA_RENDER_H
#define _MGA_RENDER_H

extern void mgaDDRenderElementsDirect( struct vertex_buffer *VB );
extern void mgaDDRenderElementsImmediate( struct vertex_buffer *VB );
extern void mgaDDRenderDirect( struct vertex_buffer *VB );
extern void mgaDDRenderInit();

#endif
