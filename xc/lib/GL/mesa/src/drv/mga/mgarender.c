#include <stdio.h>
#include "mgadd.h"
#include "mgavb.h"
#include "mgadma.h"
#include "mgalib.h"
#include "mgatris.h"
#include "mgastate.h"
#include "xsmesaP.h"
#include "enums.h"

#define POINT(x) mga_draw_point(&gWin[x], psize)
#define LINE(x,y) mga_draw_line(&gWin[x], &gWin[y], lwidth)
#define TRI(x,y,z) mga_draw_triangle(&gWin[x], &gWin[y], &gWin[z])


/* Direct, and no clipping required.  I haven't written the clip funcs
 * yet, so this is only useful for the fast path, which does its own
 * clipping.
 */
#define RENDER_POINTS( start, count )		\
do {						\
   GLuint e;					\
   for(e=start;e<=count;e++)			\
     POINT(elt[e]);				\
} while (0)

#define RENDER_LINE( i1, i )			\
do {						\
   GLuint e1 = elt[i1], e = elt[i];		\
   LINE( e1, e );				\
} while (0)


#define RENDER_TRI( i2, i1, i, pv, parity)		\
do {							\
{  GLuint e2 = elt[i2], e1 = elt[i1], e = elt[i];	\
  if (parity) {GLuint tmp = e2; e2 = e1; e1 = tmp;}	\
  TRI(e2, e1, e);					\
}} while (0)


#define RENDER_QUAD( i3, i2, i1, i, pv)				\
do {								\
  GLuint e3 = elt[i3], e2 = elt[i2], e1 = elt[i1], e = elt[i];	\
  TRI(e3, e2, e);						\
  TRI(e2, e1, e);						\
} while (0)

#define LOCAL_VARS					\
   mgaVertexPtr gWin = MGA_DRIVER_DATA(VB)->verts;	\
   const GLuint *elt = VB->EltPtr->data;		\
   const GLfloat lwidth = VB->ctx->Line.Width;		\
   const GLfloat psize = VB->ctx->Point.Size;		\
   GLcontext *ctx = VB->ctx; \
   (void) lwidth; (void)psize; (void)ctx; (void) gWin;


#define TAG(x) x##_mga_smooth_indirect
#include "render_tmp.h"




#define RENDER_POINTS( start, count )		\
do {						\
   GLuint e;					\
   for(e=start;e<=count;e++)			\
     POINT(e);					\
} while (0)

#define RENDER_LINE( i1, i )			\
do {						\
   LINE( i1, i );				\
} while (0)


#define RENDER_TRI( i2, i1, i, pv, parity)		\
do {							\
  GLuint e2 = i2, e1 = i1;				\
  if (parity) {GLuint tmp = e2; e2 = e1; e1 = tmp;}	\
  TRI(e2, e1, i);					\
} while (0)


#define RENDER_QUAD( i3, i2, i1, i, pv)		\
do {						\
  TRI(i3, i2, i);				\
  TRI(i2, i1, i);				\
} while (0)

#define LOCAL_VARS					\
   mgaVertexPtr gWin = MGA_DRIVER_DATA(VB)->verts;	\
   const GLfloat lwidth = VB->ctx->Line.Width;		\
   const GLfloat psize = VB->ctx->Point.Size;		\
   GLcontext *ctx = VB->ctx; \
   (void) lwidth; (void)psize; (void)ctx; (void) gWin;

#define TAG(x) x##_mga_smooth_direct
#include "render_tmp.h"





/* Vertex array rendering via. the SetupDma function - no clipping required. 
 */
#define RENDER_POINTS( start, count )		\
do {						\
    FatalError("Dead code in mgarender.c\n");	\
} while (0)

#define RENDER_LINE( i1, i )			\
do {						\
    FatalError("Dead code in mgarender.c\n");	\
} while (0)


#define RENDER_TRI( i2, i1, i, pv, parity)		\
do {							\
   GLuint e2 = elt[i2], e1 = elt[i1], e = elt[i];	\
   if (/*parity*/0) {GLuint tmp = e2; e2 = e1; e1 = tmp;}	\
   TRI(e2, e1, e);		\
} while (0)

#define RENDER_QUAD( i3, i2, i1, i, pv)				\
do {								\
  GLuint e3 = elt[i3], e2 = elt[i2], e1 = elt[i1], e = elt[i];	\
  TRI(e3, e2, e);			\
  TRI(e2, e1, e);			\
} while (0)

#define LOCAL_VARS					\
   mgaVertexBufferPtr mgaVB = MGA_DRIVER_DATA(VB);	\
   mgaUI32 phys = mgaVB->vert_phys_start;		\
   const GLuint *elt = VB->EltPtr->data;		\
   (void) phys; (void) elt;

#define PRESERVE_VB_DEFS
#undef TRI
#define VERT_ADDR_10( phys, elt ) ( phys + elt * 48 )
#define TRI( ee0, ee1, ee2 )				\
{							\
   mgaVB->elt_buf[0] = VERT_ADDR_10(phys, ee0);		\
   mgaVB->elt_buf[1] = VERT_ADDR_10(phys, ee1);		\
   mgaVB->elt_buf[2] = VERT_ADDR_10(phys, ee2);		\
   mgaVB->elt_buf += 3;					\
}
#define TAG(x) x##_mga_elt_10
#include "render_tmp.h"


#define TAG(x) x##_mga_elt_8
#undef TRI
#define VERT_ADDR_8( phys, elt ) ( phys + elt * 32 )
#define TRI( ee0, ee1, ee2 )			\
{						\
   mgaVB->elt_buf[0] = VERT_ADDR_8(phys, ee0);	\
   mgaVB->elt_buf[1] = VERT_ADDR_8(phys, ee1);	\
   mgaVB->elt_buf[2] = VERT_ADDR_8(phys, ee2);	\
   mgaVB->elt_buf += 3;				\
}
#include "render_tmp.h"




/* Currently only used on the fast path - need a set of render funcs
 * for clipped primitives.
 */
void mgaDDRenderElementsDirect( struct vertex_buffer *VB )
{
   mgaVertexBufferPtr mgaVB = MGA_DRIVER_DATA(VB);
   GLcontext *ctx = VB->ctx;
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLenum prim = ctx->CVA.elt_mode;
   GLuint nr = VB->EltPtr->count;
   render_func func = render_tab_mga_smooth_indirect[prim];
   GLuint p = 0;
   mgaUI32 *start = mgaVB->elt_buf;
   
   struct vertex_buffer *saved_vb = ctx->VB;

   if (mmesa->new_state)
      mgaDDUpdateHwState( ctx );

   /* Are we using Setup DMA?
    */
   if (start) {
      switch (MGA_CONTEXT(ctx)->vertsize) {
      case 10:
	 func = render_tab_mga_elt_10[prim];
	 break;
      case 8:
	 func = render_tab_mga_elt_8[prim];
	 break;
      default:
      }
   }

   ctx->VB = VB;

   do {
      func( VB, 0, nr, 0 );
   } while (ctx->Driver.MultipassFunc &&
	    ctx->Driver.MultipassFunc( VB, ++p ));

   ctx->VB = saved_vb;


   if (start && nr) {
      if (0)
	 fprintf(stderr, "%d/%d elts\n", nr, mgaVB->elt_buf-start);
      mgaSetupDma( start, mgaVB->elt_buf - start );
      mgaVB->elt_buf = mgaVB->vert_buf = 0;
   }
}


void mgaDDRenderElementsImmediate( struct vertex_buffer *VB )
{
   GLcontext *ctx = VB->ctx;
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint nr = VB->EltPtr->count;
   render_func *tab = render_tab_mga_smooth_indirect;
   GLuint parity = VB->Parity;
   GLuint p = 0;
   GLuint i, next;

   if (mmesa->new_state)
      mgaDDUpdateHwState( ctx );

   do {
      for ( i= VB->CopyStart ; i < nr ; parity = 0, i = next ) 
      {
	 GLenum prim = VB->Primitive[i];
	 next = VB->NextPrimitive[i];
	 tab[prim]( VB, i, next, parity );
      }
   } while (ctx->Driver.MultipassFunc &&
	    ctx->Driver.MultipassFunc( VB, ++p ));
}


void mgaDDRenderDirect( struct vertex_buffer *VB )
{
   GLcontext *ctx = VB->ctx;
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint nr = VB->Count;
   render_func *tab = render_tab_mga_smooth_direct;
   GLuint parity = VB->Parity;
   GLuint p = 0;
   GLuint i, next;

   if (mmesa->new_state)
      mgaDDUpdateHwState( ctx );

   do {
      for ( i= VB->CopyStart ; i < nr ; parity = 0, i = next ) 
      {
	 GLenum prim = VB->Primitive[i];
	 next = VB->NextPrimitive[i];
	 tab[prim]( VB, i, next, parity );
      }
   } while (ctx->Driver.MultipassFunc &&
	    ctx->Driver.MultipassFunc( VB, ++p ));
}


static void optimized_render_vb_triangle_mga_smooth_indirect(struct vertex_buffer *VB,
							    GLuint start,
							    GLuint count,
							    GLuint parity)
{
   mgaVertexPtr gWin = MGA_DRIVER_DATA(VB)->verts;
   const GLuint *elt = VB->EltPtr->data;
   GLcontext *ctx = VB->ctx;
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );

   mgaUI32 vertsize = mmesa->vertsize;

   /* We know how many dwords we need so we can allocate all of it
    * in one call */
   mgaUI32 *wv = mgaAllocSecondaryBuffer( count * vertsize );

   int vertex;

   (void)ctx; (void) gWin; (void) parity;


   for(vertex=start+2;vertex<count;vertex+=3){
      GLuint e2=elt[vertex-2],e1=elt[vertex-1],e=elt[vertex];
      mgaUI32 *src;

      int j;

      if(parity){
         GLuint tmp=e2;
         e2=e1;
	 e1=tmp;
      }

      src=&gWin[e2].ui[0];
      
      for(j=vertsize;j;j--){
	 *wv++=*src++;
      }
      
      src=&gWin[e1].ui[0];
      for(j=vertsize;j;j--){
	 *wv++=*src++;
      }
      
      src=&gWin[e].ui[0];
      for(j=vertsize;j;j--){
	 *wv++=*src++;
      }
   }
   
}


void mgaDDRenderInit()
{

   render_init_mga_smooth_indirect();
   render_init_mga_smooth_direct();
   render_init_mga_elt_10();
   render_init_mga_elt_8();
   
   render_tab_mga_smooth_indirect[GL_TRIANGLES] =
      optimized_render_vb_triangle_mga_smooth_indirect; 
}


