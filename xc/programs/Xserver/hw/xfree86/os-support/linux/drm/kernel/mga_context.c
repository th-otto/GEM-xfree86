/* mga_context.c -- IOCTLs for mga contexts -*- linux-c -*-
 * Created: Mon Dec 13 09:51:35 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Author: Rickard E. (Rik) Faith <faith@precisioninsight.com>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/kernel/mga_context.c,v 1.1 2000/02/11 17:26:06 dawes Exp $
 *
 */

#include <linux/sched.h>

#define __NO_VERSION__
#include "drmP.h"
#include "mga_drv.h"

static int mga_alloc_queue(drm_device_t *dev)
{
   	int temp = drm_ctxbitmap_next(dev);
   	printk("mga_alloc_queue: %d\n", temp);
	return temp;
}

int mga_context_switch(drm_device_t *dev, int old, int new)
{
        char        buf[64];

        atomic_inc(&dev->total_ctx);

        if (test_and_set_bit(0, &dev->context_flag)) {
                DRM_ERROR("Reentering -- FIXME\n");
                return -EBUSY;
        }

#if DRM_DMA_HISTOGRAM
        dev->ctx_start = get_cycles();
#endif
        
        printk("Context switch from %d to %d\n", old, new);

        if (new == dev->last_context) {
                clear_bit(0, &dev->context_flag);
                return 0;
        }
        
        if (drm_flags & DRM_FLAG_NOCTX) {
                mga_context_switch_complete(dev, new);
        } else {
                sprintf(buf, "C %d %d\n", old, new);
                drm_write_string(dev, buf);
        }
        
        return 0;
}

int mga_context_switch_complete(drm_device_t *dev, int new)
{
        dev->last_context = new;  /* PRE/POST: This is the _only_ writer. */
        dev->last_switch  = jiffies;
        
        if (!_DRM_LOCK_IS_HELD(dev->lock.hw_lock->lock)) {
                DRM_ERROR("Lock isn't held after context switch\n");
        }

				/* If a context switch is ever initiated
                                   when the kernel holds the lock, release
                                   that lock here. */
#if DRM_DMA_HISTOGRAM
        atomic_inc(&dev->histo.ctx[drm_histogram_slot(get_cycles()
                                                      - dev->ctx_start)]);
                   
#endif
        clear_bit(0, &dev->context_flag);
        wake_up(&dev->context_wait);
        
        return 0;
}

int mga_resctx(struct inode *inode, struct file *filp, unsigned int cmd,
	       unsigned long arg)
{
	drm_ctx_res_t	res;
	drm_ctx_t	ctx;
	int		i;

	printk("%d\n", DRM_RESERVED_CONTEXTS);
	copy_from_user_ret(&res, (drm_ctx_res_t *)arg, sizeof(res), -EFAULT);
	if (res.count >= DRM_RESERVED_CONTEXTS) {
		memset(&ctx, 0, sizeof(ctx));
		for (i = 0; i < DRM_RESERVED_CONTEXTS; i++) {
			ctx.handle = i;
			copy_to_user_ret(&res.contexts[i],
					 &i,
					 sizeof(i),
					 -EFAULT);
		}
	}
	res.count = DRM_RESERVED_CONTEXTS;
	copy_to_user_ret((drm_ctx_res_t *)arg, &res, sizeof(res), -EFAULT);
	return 0;
}

int mga_addctx(struct inode *inode, struct file *filp, unsigned int cmd,
	       unsigned long arg)
{
	drm_file_t	*priv	= filp->private_data;
	drm_device_t	*dev	= priv->dev;
	drm_ctx_t	ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	if ((ctx.handle = mga_alloc_queue(dev)) == DRM_KERNEL_CONTEXT) {
				/* Skip kernel's context and get a new one. */
		ctx.handle = mga_alloc_queue(dev);
	}
        if (ctx.handle == -1) {
		printk("Not enough free contexts.\n");
				/* Should this return -EBUSY instead? */
		return -ENOMEM;
	}
	printk("%d\n", ctx.handle);
	copy_to_user_ret((drm_ctx_t *)arg, &ctx, sizeof(ctx), -EFAULT);
	return 0;
}

int mga_modctx(struct inode *inode, struct file *filp, unsigned int cmd,
	unsigned long arg)
{
   	/* This does nothing for the mga */
	return 0;
}

int mga_getctx(struct inode *inode, struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	drm_ctx_t ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t*)arg, sizeof(ctx), -EFAULT);
	/* This is 0, because we don't hanlde any context flags */
	ctx.flags = 0;
	copy_to_user_ret((drm_ctx_t*)arg, &ctx, sizeof(ctx), -EFAULT);
	return 0;
}

int mga_switchctx(struct inode *inode, struct file *filp, unsigned int cmd,
		   unsigned long arg)
{
	drm_file_t	*priv	= filp->private_data;
	drm_device_t	*dev	= priv->dev;
	drm_ctx_t	ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	printk("%d\n", ctx.handle);
	return mga_context_switch(dev, dev->last_context, ctx.handle);
}

int mga_newctx(struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	drm_file_t	*priv	= filp->private_data;
	drm_device_t	*dev	= priv->dev;
	drm_ctx_t	ctx;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	printk("%d\n", ctx.handle);
	mga_context_switch_complete(dev, ctx.handle);

	return 0;
}

int mga_rmctx(struct inode *inode, struct file *filp, unsigned int cmd,
	      unsigned long arg)
{
	drm_file_t	*priv	= filp->private_data;
	drm_device_t	*dev	= priv->dev;
	drm_ctx_t	ctx;
	drm_queue_t	*q;
	drm_buf_t	*buf;

	copy_from_user_ret(&ctx, (drm_ctx_t *)arg, sizeof(ctx), -EFAULT);
	printk("%d\n", ctx.handle);
	if(ctx.handle == DRM_KERNEL_CONTEXT) {
	   q = dev->queuelist[ctx.handle];
	   atomic_inc(&q->use_count);
	   if (atomic_read(&q->use_count) == 1) {
	      /* No longer in use */
	      atomic_dec(&q->use_count);
	      return -EINVAL;
	   }
	   atomic_inc(&q->finalization); /* Mark queue in finalization state */
	   atomic_sub(2, &q->use_count); 
	   /* Mark queue as unused (pending finalization) */
	   
	   while (test_and_set_bit(0, &dev->interrupt_flag)) {
	      printk("Calling schedule from rmctx\n");
	      schedule();
	      if (signal_pending(current)) {
		 clear_bit(0, &dev->interrupt_flag);
		 return -EINTR;
	      }
	   }
	   
	   /* Remove queued buffers */
	   while ((buf = drm_waitlist_get(&q->waitlist))) {
	      drm_free_buffer(dev, buf);
	   }
	   clear_bit(0, &dev->interrupt_flag);
	   
	   /* Wakeup blocked processes */
	   wake_up_interruptible(&q->read_queue);
	   wake_up_interruptible(&q->write_queue);
	   wake_up_interruptible(&q->flush_queue);
	   
	   /* Finalization over.  Queue is made
	    available when both use_count and
	    finalization become 0, which won't
	    happen until all the waiting processes
	    stop waiting. */
	   atomic_dec(&q->finalization);
	} else {
	   drm_ctxbitmap_free(dev, ctx.handle);
	}
	
	return 0;
}
