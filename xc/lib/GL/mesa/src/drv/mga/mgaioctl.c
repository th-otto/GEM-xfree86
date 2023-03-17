#include <stdio.h>


#include "types.h"
#include "pb.h"
#include "dd.h"

#include "mm.h"
#include "mgalib.h"
#include "mgadd.h"
#include "mgastate.h"
#include "mgadepth.h"
#include "mgatex.h"
#include "mgalog.h"
#include "mgavb.h"
#include "mgatris.h"

#include "drm.h"
#include <sys/ioctl.h>

static void mga_iload_dma_ioctl(mgaContextPtr mmesa,
				int x1, int y1, int x2, int y2, 
				unsigned long dest, unsigned int maccess)
{
   int retcode;
   drm_mga_iload_t iload;
   drmBufPtr buf = mmesa->dma_buffer;
   
   iload.idx = buf->idx;
   iload.destOrg = dest;	
   iload.mAccess = maccess;
   iload.texture.x1 = x1;
   iload.texture.y1 = y1;
   iload.texture.y2 = x2;
   iload.texture.x2 = y2;
   
   if ((retcode = ioctl(mmesa->driFd, DRM_IOCTL_MGA_ILOAD, &iload))) {
      printf("send iload retcode = %d\n", retcode);
      exit(1);
   }
}


static void mga_vertex_dma_ioctl(mgaContextPtr mmesa)
{
   int retcode;
   int size = MGA_DMA_BUF_SZ;
   drmDMAReq dma;
   drmBufPtr buf = mmesa->dma_buffer;

   dma.context = mmesa->hHWContext;
   dma.send_count = 1;
   dma.send_list = &buf->idx;
   dma.send_sizes = &size;
   dma.flags = DRM_DMA_WAIT;
   dma.request_count = 0;
   dma.request_size = 0;
   dma.request_list = 0;
   dma.request_sizes = 0;

   if ((retcode = drmDMA(mmesa->driFd, &dma))) {
      printf("send iload retcode = %d\n", retcode);
      exit(1);
   }
}


static void mga_get_buffer_ioctl( mgaContextPtr mmesa )
{
   int idx = 0;
   int size = 0;
   drmDMAReq dma;
   int retcode;
   
   fprintf(stderr,  "Getting dma buffer\n");
   
   dma.context = mmesa->hHWContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = DRM_DMA_WAIT;
   dma.request_count = 1;
   dma.request_size = MGA_DMA_BUF_SZ;
   dma.request_list = &idx;
   dma.request_sizes = &size;
   
   if ((retcode = drmDMA(mmesa->driFd, &dma))) {
      fprintf(stderr, "request drmDMA retcode = %d\n", retcode);
      exit(1);
   }

   mmesa->dma_buffer = &mmesa->mgaScreen->bufs->list[idx];
}

static void mga_swap_ioctl( mgaContextPtr mmesa )
{
   int retcode;
   drm_mga_swap_t swap;
      
   if((retcode = ioctl(mmesa->driFd, DRM_IOCTL_MGA_SWAP, &swap))) {
      printf("send swap retcode = %d\n", retcode);
      exit(1);
   }
}



static void mga_clear_ioctl( mgaContextPtr mmesa, int flags, int col, int zval )
{
   int retcode;
   drm_mga_clear_t clear;
   
   clear.flags = flags;
   clear.clear_color = col;
   clear.clear_depth = zval;
      
   if((retcode = ioctl(mmesa->driFd, DRM_IOCTL_MGA_CLEAR, &clear))) {
      printf("send clear retcode = %d\n", retcode);
      exit(1);
   }
}







GLbitfield mgaClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		     GLint cx, GLint cy, GLint cw, GLint ch ) 
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   GLuint c = mmesa->ClearColor;
   mgaUI32 zval = (mgaUI32) (ctx->Depth.Clear * DEPTH_SCALE);
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   int flags = 0;
   int i;

   mgaMsg( 10, "mgaClear( %i, %i, %i, %i, %i )\n", 
	   mask, x, y, width, height );


   mgaFlushVertices( mmesa );


   if (mask & GL_COLOR_BUFFER_BIT) {
      if (ctx->Color.DriverDrawBuffer == GL_FRONT_LEFT) {
	 flags |= MGA_CLEAR_FRONT;
	 mask &= ~GL_COLOR_BUFFER_BIT;
      } else if (ctx->Color.DriverDrawBuffer == GL_BACK_LEFT) {
	 flags |= MGA_CLEAR_BACK;
	 mask &= ~GL_COLOR_BUFFER_BIT;
      }
   }

   if ((flags & GL_DEPTH_BUFFER_BIT) && ctx->Depth.Mask) {
      flags |= MGA_CLEAR_DEPTH;
      mask &= ~GL_DEPTH_BUFFER_BIT;
   }

   if (!flags)
      return mask;

   LOCK_HARDWARE( mmesa );
   
   /* flip top to bottom */
   cy = dPriv->h-cy-ch;
   cx += mmesa->drawX;
   cy += mmesa->drawY;

   for (i = 0 ; i < dPriv->numClipRects ; ) { 	 

      /* Use the cliprects for the current draw buffer
       */
      int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, mmesa->numClipRects);
      XF86DRIClipRectRec *box = mmesa->pClipRects;	 
      xf86drmClipRectRec *b = mmesa->sarea->boxes;
      mmesa->sarea->nbox = nr - i;

      if (!all) {
	 for ( ; i < nr ; i++) {
	    GLint x = box[i].x1;
	    GLint y = box[i].y1;
	    GLint w = box[i].x2 - x;
	    GLint h = box[i].y2 - y;

	    if (x < cx) w -= cx - x, x = cx; 
	    if (y < cy) h -= cy - y, y = cy;
	    if (x + w > cx + cw) w = cx + cw - x;
	    if (y + h > cy + ch) h = cy + ch - y;
	    if (w <= 0) continue;
	    if (h <= 0) continue;

	    b->x1 = x;
	    b->y1 = y;
	    b->x2 = x + w;
	    b->y2 = y + h;
	    b++;
	 }
      } else {
	 for ( ; i < nr ; i++) 
	    *b++ = *(xf86drmClipRectRec *)&box[i];
      }

      mga_clear_ioctl( mmesa, mask, c, zval );
   }

   UNLOCK_HARDWARE( mmesa );
   return mask;
}




/*
 * Copy the back buffer to the front buffer. 
 */
void mgaSwapBuffers( mgaContextPtr mmesa ) 
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   int i;

   mgaFlushVertices( mmesa );

   LOCK_HARDWARE( mmesa );
   {
      /* Use the frontbuffer cliprects
       */
      XF86DRIClipRectPtr pbox = dPriv->pClipRects;
      int nbox = dPriv->numClipRects;

      for (i = 0 ; i < nbox ; )
      {
	 int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
	 XF86DRIClipRectRec *b = (XF86DRIClipRectRec *)mmesa->sarea->boxes;
	 mmesa->sarea->nbox = nr - i;

	 for ( ; i < nr ; i++) 
	    *b++ = pbox[i];
	 
	 mga_swap_ioctl( mmesa );
      }
   }


#if 1
   UNLOCK_HARDWARE(mmesa);
#else
   {
      last_enqueue = mmesa->sarea->lastEnqueue;
      last_dispatch = mmesa->sarea->lastDispatch;
      UNLOCK_HARDWARE;
      
      /* Throttle runaway apps - there should be an easier way to sleep
       * on dma without locking out the rest of the system!
       */
      if (mmesa->lastSwap > last_dispatch) {
	 drmGetLock(mmesa->driFd, mmesa->hHWContext, DRM_LOCK_QUIESCENT);	
	 DRM_UNLOCK(mmesa->driFd, mmesa->driHwLock, mmesa->hHWContext);	
      }

      mmesa->lastSwap = last_enqueue;
   }
#endif
}


/* This is overkill
 */
void mgaDDFinish( GLcontext *ctx  ) 
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   drmGetLock(mmesa->driFd, mmesa->hHWContext, DRM_LOCK_QUIESCENT);	
   DRM_UNLOCK(mmesa->driFd, mmesa->driHwLock, mmesa->hHWContext);	
}




void mgaFlushVertices( mgaContextPtr mmesa ) 
{
   LOCK_HARDWARE( mmesa );
   mgaFlushVerticesLocked( mmesa );
   UNLOCK_HARDWARE( mmesa );
}


void mgaFlushVerticesLocked( mgaContextPtr mmesa )
{
   XF86DRIClipRectPtr pbox = mmesa->pClipRects;
   int nbox = mmesa->numClipRects;
   int i;

   if (mmesa->dirty)
      mgaEmitHwStateLocked( mmesa );

   for (i = 0 ; i < nbox ; )
   {
      int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, nbox);
      XF86DRIClipRectRec *b = (XF86DRIClipRectRec *)mmesa->sarea->boxes;
      mmesa->sarea->nbox = nr - i;

      for ( ; i < nr ; i++) 
	 *b++ = pbox[i];
	 
      mga_vertex_dma_ioctl( mmesa );

      break;			/* fix dma multiple dispatch */
   }

   mga_get_buffer_ioctl( mmesa );
}



void mgaDDInitIoctlFuncs( GLcontext *ctx )
{
   ctx->Driver.Clear = mgaClear;
   ctx->Driver.Flush = 0;
   ctx->Driver.Finish = mgaDDFinish;
}
