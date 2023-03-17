

#include "types.h"
#include "vbrender.h"
#include "i810log.h"

#include <stdio.h>
#include <stdlib.h>

#include "mm.h"
#include "i810lib.h"
#include "i810dd.h"
#include "i810dma.h"
#include "i810state.h"
#include "i810swap.h"
#include "i810dma.h"

/*
 * i810BackToFront
 *
 * Blit the visible rectangles from the back buffer to the screen.
 * Respects the frontbuffer cliprects, and applies the offset
 * necessary if the backbuffer is positioned elsewhere in the screen.
 */
static int i810BackToFront( i810ContextPtr imesa ) 
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   i810ScreenPrivate *i810Screen = imesa->i810Screen;


   /* Use the frontbuffer cliprects:
    */
   XF86DRIClipRectPtr pbox = dPriv->pClipRects;
   int nbox = dPriv->numClipRects;

   if( nbox ) 
   {
      int i;
      int pitch = i810Screen->auxPitch;

      int dx = dPriv->auxX - dPriv->x;
      int dy = dPriv->auxY - dPriv->y;
      int ofs = ( i810Screen->backOffset + 
		  dy * i810Screen->auxPitch +
		  dx * i810Screen->cpp);

      unsigned int BR13 = ((i810Screen->fbStride) |
			   (0xCC << 16));


      for (i=0; i < nbox; i++, pbox++) {
	 int w = pbox->x2 - pbox->x1;
	 int h = pbox->y2 - pbox->y1;

	 int start = ofs + pbox->x1*i810Screen->cpp + pbox->y1*pitch;
	 int dst = pbox->x1*i810Screen->cpp + pbox->y1*i810Screen->fbStride;
	 
	 if (I810_DEBUG&DEBUG_VERBOSE_2D) 
	    fprintf(stderr, "i810BackToFront %d,%d-%dx%d\n",
		    pbox->x1,pbox->y1,w,h);

	 {
	    BEGIN_BATCH( imesa, 6 );
	    OUT_BATCH( BR00_BITBLT_CLIENT | BR00_OP_SRC_COPY_BLT | 0x4 );
	    OUT_BATCH( BR13 );	/* dest pitch, rop */

	    OUT_BATCH( (h << 16) | (w * i810Screen->cpp));
	    OUT_BATCH( dst );

	    OUT_BATCH( pitch );	/* src pitch */
	    OUT_BATCH( start );
	    ADVANCE_BATCH();
	 }
      }
   }
  
   return Success;   
}


/*
 * ClearBox
 *
 * Add hardware commands to draw a filled box for the debugging
 * display.  These are drawn into the current drawbuffer, so should
 * work with both front and backbuffer rendering.  (However, it
 * is only ever called on swapbuffer - ie backbuffer rendering).
 */
static void ClearBox( i810ContextPtr imesa,
		      int cx, int cy, int cw, int ch, 
		      int r, int g, int b ) 
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   i810ScreenPrivate *i810Screen = imesa->i810Screen;

   int _nc = imesa->numClipRects;
      
   while (_nc--) {
      int x1 = cx + dPriv->x;
      int y1 = cy + dPriv->y;
      int x2 = x1 + cw;
      int y2 = y1 + ch;

      if (imesa->needClip) {
	 int rx1 = imesa->pClipRects[_nc].x1;
	 int ry1 = imesa->pClipRects[_nc].y1;
	 int rx2 = imesa->pClipRects[_nc].x2;
	 int ry2 = imesa->pClipRects[_nc].y2;


	 if (x2 < rx1) continue;
	 if (y2 < ry1) continue;
	 if (x1 > rx2) continue;
	 if (y1 > ry2) continue;
	 if (x1 < rx1) x1 = rx1;
	 if (y1 < ry1) y1 = ry1;
	 if (x2 > rx2) x2 = rx2;
	 if (y2 > ry2) y2 = ry2;
      }


      {
	 int start = (imesa->drawOffset + 
		      y1 * i810Screen->auxPitch + 
		      x1 * i810Screen->cpp);

	 BEGIN_BATCH( imesa, 6 );
   
	 OUT_BATCH( BR00_BITBLT_CLIENT | BR00_OP_COLOR_BLT | 0x3 );
	 OUT_BATCH( BR13_SOLID_PATTERN | 
		    (0xF0 << 16) |
		    i810Screen->auxPitch );
	 OUT_BATCH( ((y2-y1) << 16) | ((x2-x1) * i810Screen->cpp));
	 OUT_BATCH( start );
	 OUT_BATCH( i810PackColor( i810Screen->fbFormat, r, g, b, 0 ) );
	 OUT_BATCH( 0 );

	 if (I810_DEBUG&DEBUG_VERBOSE_2D) 
	    fprintf(stderr, "box %d,%x %d,%d col %x (%d,%d,%d)\n", 
		    (int)x1,(int)y1, (int)x2,(int)y2,
		    (int)i810PackColor( i810Screen->fbFormat, r, g, b, 0 ), 
		    (int)r, (int)g, (int)b);
	 
	 ADVANCE_BATCH();
      }
   }
}


/*
 * performanceBoxes
 * Draw some small boxesin the corner of the buffer
 * based on some performance information
 */
static void i810PerformanceBoxes( i810ContextPtr imesa ) {
	

   /* draw a box to show we are active, so if it is't seen it means
      that it is completely software based.  Purple is traditional for
      direct rendering */
   ClearBox( imesa, 4, 4, 8, 8, 255, 0, 255 );
	
   /* draw a red box if we had to wait for drawing to complete 
      (software render or texture swap) */
   if ( i810glx.c_drawWaits ) {
      ClearBox( imesa, 16, 4, 8, 8, 255, 0, 0 );
      i810glx.c_drawWaits = 0;
   }
	
   /* draw a blue box if the 3d context was lost
    */
   if ( i810glx.c_ctxlost ) {
      ClearBox( imesa, 28, 4, 8, 8, 0, 0, 255 );
      i810glx.c_ctxlost = 0;
   }
	
   /* draw an orange box if texture state was stomped */
   if ( i810glx.c_texlost ) {
      ClearBox( imesa, 40, 4, 8, 8, 255, 128, 0 );
      i810glx.c_texlost = 0;
   }

   /* draw a yellow box if textures were swapped */
   if ( i810glx.c_textureSwaps ) {
      ClearBox( imesa, 56, 4, 8, 8, 255, 255, 0 );
      i810glx.c_textureSwaps = 0;
   }
		
   /* draw a green box if we had to wait for dma to complete (full utilization) 
      on the previous frame */
   if ( !i810glx.hardwareWentIdle ) {
      ClearBox( imesa, 72, 4, 8, 8, 0, 255, 0 );
   }
   i810glx.hardwareWentIdle = 0;
	
#if 0
   /* show buffer utilization */
   if ( i810glx.c_dmaFlush > 1 ) {
      /* draw a solid bar if we flushed more than one buffer */
      ClearBox( imesa, 4, 16, 252, 4, 255, 32, 32 );
   } else {
      /* draw bars to represent the utilization of primary and
         secondary buffers */
      ClearBox( imesa, 4, 16, 252, 4, 32, 32, 32 );
      t = i810glx.dma_buffer->mem.Size;
      w = 252 * i810glx.dma_buffer->head / t;
      if ( w < 1 ) {
	 w = 1;
      }
      ClearBox( imesa, 4, 16, w, 4, 196, 128, 128 );
   }
#endif

   i810glx.c_dmaFlush = 0;
}


static void i810SendDmaFlush( i810ContextPtr imesa )
{
   BEGIN_BATCH( imesa, 2 );
   OUT_BATCH( INST_PARSER_CLIENT | INST_OP_FLUSH );
   OUT_BATCH( 0 );
   ADVANCE_BATCH();
}

void i810SwapBuffers( i810ContextPtr imesa )
{
   if (I810_DEBUG & DEBUG_VERBOSE_API)
      fprintf(stderr, "i810SwapBuffers()\n");

   LOCK_HARDWARE( imesa );
   i810SendDmaFlush( imesa );
   if ( i810glx.boxes )  
      i810PerformanceBoxes( imesa );
   i810BackToFront( imesa );

   /* Check if we are emitting thousands of tiny frames, and if so
    * throttle them.  Would be better to simply go to sleep until
    * our stuff had cleared, rather than polling.  A dma solution
    * should provide for this.
    */   
   if (imesa->lastSwap >= imesa->sarea->lastWrap &&
       imesa->lastSwap >= imesa->sarea->lastSync) {

      if (I810_DEBUG & DEBUG_VERBOSE_API)
	 fprintf(stderr, ".");

      _I810Sync( imesa );
   }

   imesa->lastSwap = imesa->sarea->lastSync;
   if (imesa->sarea->lastWrap > imesa->lastSwap) 
      imesa->lastSwap = imesa->sarea->lastWrap;
      
   
   UNLOCK_HARDWARE( imesa );

/*     XSync(imesa->display, 0); */
}
