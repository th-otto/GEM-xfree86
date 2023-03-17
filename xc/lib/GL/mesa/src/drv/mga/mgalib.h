/*
 * GLX Hardware Device Driver for Matrox Millenium G200
 * Copyright (C) 1999 Wittawat Yamwong
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * WITTAWAT YAMWONG, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    Wittawat Yamwong <Wittawat.Yamwong@stud.uni-hannover.de>
 */


#ifndef MGALIB_INC
#define MGALIB_INC

#include <X11/Xlibint.h>
#include "dri_tmm.h"
#include "dri_mesaint.h"
#include "dri_mesa.h"
#include "xmesaP.h"

#include "types.h"

#include "mgacommon.h"
#include "mm.h"
#include "mgalog.h"
#include "mgaioctl.h"
#include "mgatex.h"
#include "mgavb.h"

#include "mga_drm_public.h"
#include "mga_xmesa.h"


#define MGA_SET_FIELD(reg,mask,val)  reg = ((reg) & (mask)) | ((val) & ~(mask))
#define MGA_FIELD(field,val) (((val) << (field ## _SHIFT)) & ~(field ## _MASK))
#define MGA_GET_FIELD(field, val) ((val & ~(field ## _MASK)) >> (field ## _SHIFT))

#define MGA_CHIP_MGAG200 0
#define MGA_CHIP_MGAG400 1

#define MGA_IS_G200(mmesa) (mmesa->mgaScreen->chipset == MGA_CHIP_MGAG200)
#define MGA_IS_G400(mmesa) (mmesa->mgaScreen->chipset == MGA_CHIP_MGAG400)


/* SoftwareFallback 
 *    - texture env GL_BLEND -- can be fixed 
 *    - 1D and 3D textures
 *    - incomplete textures
 */
#define MGA_FALLBACK_TEXTURE   0x1
#define MGA_FALLBACK_BUFFER    0x2


/* For mgaCtx->new_state.
 */
#define MGA_NEW_DEPTH   0x1
#define MGA_NEW_ALPHA   0x2
#define MGA_NEW_FOG     0x4
#define MGA_NEW_CLIP    0x8
#define MGA_NEW_MASK    0x10
#define MGA_NEW_TEXTURE 0x20
#define MGA_NEW_CULL    0x40
#define MGA_NEW_WARP    0x80
#define MGA_NEW_CONTEXT 0x100


typedef void (*mga_interp_func)( GLfloat t, 
				 GLfloat *result,
				 const GLfloat *in,
				 const GLfloat *out );
				 



/* if type == MGA_COLORBUFFER */
#define MGA_PF_MASK  0xf0
#define MGA_PF_INDEX 0
#define MGA_PF_565   (1 << 4)
#define MGA_PF_555   (9 << 4)
#define MGA_PF_888   (3 << 4)
#define MGA_PF_8888  (10 << 4)
#define MGA_PF_HASALPHA (8 << 4)

struct mga_context_t {

   GLcontext *glCtx;

   /* Hardware state - moved from mgabuf.h
    */
   mgaUI32 Setup[MGA_CTX_SETUP_SIZE];

   /* Variable sized vertices
    */
   mgaUI32 vertsize;

   /* Map GL texture units onto hardware.
    */
   mgaUI32 multitex;
   mgaUI32 tmu_source[2];
   mgaUI32 tex_dest[2];



   /* bookkeeping for textureing */
   struct mga_texture_object_s TexObjList;
   struct mga_texture_object_s SwappedOut;
   struct mga_texture_object_s *CurrentTexObj[2];


   /* shared texture palette */
   mgaUI16	GlobalPalette[256];
  
   int Fallback;  /* or'ed values of FALLBACK_* */

   /* Support for CVA and the fast paths */
   unsigned int setupdone;
   unsigned int setupindex;
   unsigned int renderindex;
   unsigned int using_fast_path;
   unsigned int using_immediate_fast_path;
   mga_interp_func interp;

   /* Shortcircuit some state changes */
   points_func   PointsFunc;
   line_func     LineFunc;
   triangle_func TriangleFunc;
   quad_func     QuadFunc;

   /* Manage our own state */
   GLuint        new_state; 
   GLuint        dirty;

   GLubyte       clearcolor[4];
   GLushort MonoColor;
   GLushort ClearColor;

   
   /* DRI stuff
    */
   drmBufPtr  dma_buffer;

   GLframebuffer *glBuffer;
   memHeap_t *texHeap;

   GLuint needClip;
   GLuint warp_pipe;

   /* These refer to the current draw (front vs. back) buffer:
    */
   int drawOffset;		/* draw buffer address in agp space */
   int drawX;			/* origin of drawable in draw buffer */
   int drawY;
   GLuint numClipRects;		/* cliprects for that buffer */
   XF86DRIClipRectPtr pClipRects;

   int texAge;

   XF86DRIClipRectRec draw_rect;

   drmContext hHWContext;
   drmLock *driHwLock;
   int driFd;
   Display *display;

   __DRIdrawablePrivate *driDrawable;
   __DRIscreenPrivate *driScreen;
   mgaScreenPrivate *mgaScreen; 
   drm_mga_sarea_t *sarea;
};




typedef struct {

	/* dma stuff */
        mgaUI32         systemTexture;
        mgaUI32         noSetupDma;

        mgaUI32         default32BitTextures;
        mgaUI32         swapBuffersCount;
        
	/* options */
	mgaUI32		nullprims;  /* skip all primitive generation */
	mgaUI32		noFallback; /* don't fall back to software, do
                                       best-effort rendering */
 	mgaUI32		skipDma;    /* don't send anything to the hardware */
 	
	/* performance counters */	
	mgaUI32		c_textureUtilization;
	mgaUI32		c_textureSwaps;
	mgaUI32		c_setupPointers;
	mgaUI32		c_triangles;
	mgaUI32		c_points;
	mgaUI32		c_lines;
	mgaUI32		c_drawWaits;
	mgaUI32		c_dmaFlush;
	mgaUI32		c_overflows;

} mgaGlx_t;

extern mgaGlx_t	mgaglx;


#define MGAPACKCOLOR555(r,g,b,a) \
  ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | \
    ((a) ? 0x8000 : 0))

#define MGAPACKCOLOR565(r,g,b) \
  ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define MGAPACKCOLOR888(r,g,b) \
  (((r) << 16) | ((g) << 8) | (b))

#define MGAPACKCOLOR8888(r,g,b,a) \
  (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define MGAPACKCOLOR4444(r,g,b,a) \
  ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))


#define MGA_DEBUG 0   
#ifndef MGA_DEBUG
extern int MGA_DEBUG;
#endif

#define MGA_DEBUG_ALWAYS_SYNC    0x1
#define MGA_DEBUG_VERBOSE_MSG    0x2
#define MGA_DEBUG_VERBOSE_LRU    0x4
#define MGA_DEBUG_VERBOSE_DRI    0x8

static __inline__ mgaUI32 mgaPackColor(mgaUI32 format,
				       mgaUI8 r, mgaUI8 g, 
				       mgaUI8 b, mgaUI8 a)
{
  switch (format & MGA_PF_MASK) {
  case MGA_PF_555:
    return MGAPACKCOLOR555(r,g,b,a);
  case MGA_PF_565:
    return MGAPACKCOLOR565(r,g,b);
  case MGA_PF_888:
    return MGAPACKCOLOR888(r,g,b);
  case MGA_PF_8888:
    return MGAPACKCOLOR8888(r,g,b,a);
  default:
    return 0;
  }
}

#endif
