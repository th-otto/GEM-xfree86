

#include "types.h"
#include "vbrender.h"
#include "i810log.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mm.h"
#include "i810lib.h"
#include "i810dd.h"
#include "i810clear.h"
#include "i810state.h"
#include "i810tris.h"



/* Clear the depthbuffer.  Always uses the auxillary cliprects and
 * origin from 'dPriv'.
 */
static void i810_clear_depthbuffer( GLcontext *ctx, 
				    GLboolean all,
				    GLint cx, GLint cy, 
				    GLint cwidth, GLint cheight ) 
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLuint zval = (GLuint) (ctx->Depth.Clear * DEPTH_SCALE);
   i810ScreenPrivate *i810Screen = imesa->i810Screen;
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

/*     int _nc = dPriv->numAuxClipRects; */
/*     XF86DRIClipRectPtr box = dPriv->pAuxClipRects; */

   int _nc = imesa->numClipRects;
   XF86DRIClipRectPtr box = imesa->pClipRects;

   cy = dPriv->h-cy-cheight;
   cx += dPriv->auxX;
   cy += dPriv->auxY;
         
   while (_nc--) {	 
      GLint x = box[_nc].x1;
      GLint y = box[_nc].y1;
      GLint width = box[_nc].x2 - x;
      GLint height = box[_nc].y2 - y;

      if (!all) {
	 if (x < cx) width -= cx - x, x = cx; 
	 if (y < cy) height -= cy - y, y = cy;
	 if (x + width > cx + cwidth) width = cx + cwidth - x;
	 if (y + height > cy + cheight) height = cy + cheight - y;
	 if (width <= 0) continue;
	 if (height <= 0) continue;
      }

      if (I810_DEBUG&DEBUG_VERBOSE_2D)
	 fprintf(stderr, "clear depth rect %d,%d %dx%d\n", 
		 (int) x, (int) y, (int) width, (int) height);

      {
	 int start = (i810Screen->depthOffset + 
		      y * i810Screen->auxPitch + 
		      x * 2);
	    
	 BEGIN_BATCH(imesa, 6);
   
	 OUT_BATCH( BR00_BITBLT_CLIENT | BR00_OP_COLOR_BLT | 0x3 );
	 OUT_BATCH( BR13_SOLID_PATTERN | 
		    (0xF0 << 16) |
		    i810Screen->auxPitch );
	 OUT_BATCH( (height << 16) | (width * 2));
	 OUT_BATCH( start );
	 OUT_BATCH( zval );
	 OUT_BATCH( 0 );

	 ADVANCE_BATCH();   
      }
   }
}


/* Clear the current drawbuffer.  Checks 'imesa' to determine which set of
 * cliprects and origin to use.
 */
static void i810_clear_colorbuffer( GLcontext *ctx, 
				    GLboolean all,
				    GLint cx, GLint cy, 
				    GLint cwidth, GLint cheight ) 
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );
   GLushort c = imesa->ClearColor;
   i810ScreenPrivate *i810Screen = imesa->i810Screen;
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   int _nc = imesa->numClipRects;
   XF86DRIClipRectPtr box = imesa->pClipRects;

   cy = dPriv->h-cy-cheight;
   cx += imesa->drawX;
   cy += imesa->drawY;
         
   while (_nc--) {	 
      GLint x = box[_nc].x1;
      GLint y = box[_nc].y1;
      GLint width = box[_nc].x2 - x;
      GLint height = box[_nc].y2 - y;

      if (!all) {
	 if (x < cx) width -= cx - x, x = cx; 
	 if (y < cy) height -= cy - y, y = cy;
	 if (x + width > cx + cwidth) width = cx + cwidth - x;
	 if (y + height > cy + cheight) height = cy + cheight - y;
	 if (width <= 0) continue;
	 if (height <= 0) continue;
      }

      if (I810_DEBUG&DEBUG_VERBOSE_2D)
	 fprintf(stderr, "clear color rect %d,%d %dx%d\n", 
		 (int) x, (int) y, (int) width, (int) height);


      {
	 int start = (imesa->drawOffset +  
		      y * i810Screen->auxPitch + 
		      x * i810Screen->cpp);
	    
	 BEGIN_BATCH( imesa, 6 );
	    
	 OUT_BATCH( BR00_BITBLT_CLIENT | BR00_OP_COLOR_BLT | 0x3 );
	 OUT_BATCH( BR13_SOLID_PATTERN | 
		    (0xF0 << 16) |
		    i810Screen->auxPitch );
	 OUT_BATCH( (height << 16) | (width * i810Screen->cpp));
	 OUT_BATCH( start );
	 OUT_BATCH( c );
	 OUT_BATCH( 0 );

	 ADVANCE_BATCH();
      }
   }
}




/*
 * i810Clear
 *
 * Clear the color and/or depth buffers.  If 'all' is GL_TRUE, clear
 * whole buffer, otherwise clear the region defined by the remaining
 * parameters.  
 */
GLbitfield i810Clear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		      GLint cx, GLint cy, GLint cwidth, GLint cheight ) 
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );

   if (I810_DEBUG&DEBUG_VERBOSE_API)
      fprintf(stderr, "i810Clear( %d, %d, %d, %d, %d )\n",
	      (int)mask, (int)cx, (int)cy, (int)cwidth, (int)cheight );
	
   LOCK_HARDWARE(imesa);

   {
      BEGIN_BATCH( imesa, 2 );   
      OUT_BATCH( INST_PARSER_CLIENT | INST_OP_FLUSH );
      OUT_BATCH( 0 );
      ADVANCE_BATCH();      
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      i810_clear_colorbuffer( ctx, all, cx, cy, cwidth, cheight );
      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   if ( (mask & GL_DEPTH_BUFFER_BIT) && ctx->Depth.Mask ) {
      i810_clear_depthbuffer( ctx, all, cx, cy, cwidth, cheight );
      mask &= ~GL_DEPTH_BUFFER_BIT;
   }      

   {
      BEGIN_BATCH( imesa, 2 );   
      OUT_BATCH( INST_PARSER_CLIENT | INST_OP_FLUSH );
      OUT_BATCH( 0 );
      ADVANCE_BATCH();      
   }

   UNLOCK_HARDWARE( imesa );
   return mask;
}

