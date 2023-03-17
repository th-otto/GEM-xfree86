#include "types.h"
#include "i810dd.h"
#include "i810lib.h"
#include "i810dma.h"
#include "i810log.h"
#include "i810span.h"


#define DBG 0


#define LOCAL_VARS					\
   i810ContextPtr imesa = I810_CONTEXT(ctx);		\
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;	\
   __DRIscreenPrivate *sPriv = imesa->driScreen;	\
   i810ScreenPrivate *i810Screen = imesa->i810Screen;	\
   GLuint pitch = i810Screen->auxPitch;			\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(sPriv->pFB + 			\
			imesa->drawOffset +	\
			dPriv->x * 2 + 			\
			dPriv->y * pitch)

#define INIT_MONO_PIXEL(p) \
   GLushort p = I810_CONTEXT( ctx )->MonoColor;

#define CLIPPIXEL(_x,_y) (_x >= minx && _x <= maxx && \
			  _y >= miny && _y <= maxy)


#define CLIPSPAN(_x,_y,_n,_x1,_n1,_i)				\
	 if (_y < miny || _y >= maxy) _n1 = 0, _x1 = x;		\
         else {							\
            _n1 = _n;						\
	    _x1 = _x;						\
	    if (_x1 < minx) _i += (minx - _x1), _x1 = minx;	\
	    if (_x1 + _n1 > maxx) n1 -= (_x1 + n1 - maxx);	\
         }

#define HW_CLIPLOOP()						\
  do {								\
    __DRIdrawablePrivate *dPriv = imesa->driDrawable;		\
    int _nc = dPriv->numClipRects;				\
    LOCK_HARDWARE(imesa);						\
    if (!i810glx.LpRing.synced) _I810Sync( imesa );			\
    while (_nc--) {						\
       int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
       int miny = dPriv->pClipRects[_nc].y1 - dPriv->y; 	\
       int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
       int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;


#define HW_ENDCLIPLOOP()			\
    }						\
    UNLOCK_HARDWARE(imesa);			\
  } while (0)




#define Y_FLIP(_y) (height - _y)
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( (((int)r & 0xf8) << 8) |	\
		                             (((int)g & 0xfc) << 3) |	\
		                             (((int)b & 0xf8) >> 3))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 8) & 0xf8;				\
   rgba[1] = (p >> 3) & 0xfc;				\
   rgba[2] = (p << 3) & 0xf8;				\
   rgba[3] = 0;			/* or 255? */		\
} while(0)

#define TAG(x) i810##x##_565
#include "spantmp.h"





#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = (((r & 0xf8) << 7) |	\
		                            ((g & 0xf8) << 3) |		\
                         		    ((b & 0xf8) >> 3))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 7) & 0xf8;				\
   rgba[1] = (p >> 3) & 0xf8;				\
   rgba[2] = (p << 3) & 0xf8;				\
   rgba[3] = 0;			/* or 255? */		\
} while(0)

#define TAG(x) i810##x##_555
#include "spantmp.h"


void i810DDInitSpanFuncs( GLcontext *ctx )
{
   if (1) {
      ctx->Driver.WriteRGBASpan = i810WriteRGBASpan_565;
      ctx->Driver.WriteRGBSpan = i810WriteRGBSpan_565;
      ctx->Driver.WriteMonoRGBASpan = i810WriteMonoRGBASpan_565;
      ctx->Driver.WriteRGBAPixels = i810WriteRGBAPixels_565;
      ctx->Driver.WriteMonoRGBAPixels = i810WriteMonoRGBAPixels_565; 
      ctx->Driver.ReadRGBASpan = i810ReadRGBASpan_565;
      ctx->Driver.ReadRGBAPixels = i810ReadRGBAPixels_565;
   } else {
      ctx->Driver.WriteRGBASpan = i810WriteRGBASpan_555;
      ctx->Driver.WriteRGBSpan = i810WriteRGBSpan_555;
      ctx->Driver.WriteMonoRGBASpan = i810WriteMonoRGBASpan_555;
      ctx->Driver.WriteRGBAPixels = i810WriteRGBAPixels_555;
      ctx->Driver.WriteMonoRGBAPixels = i810WriteMonoRGBAPixels_555;
      ctx->Driver.ReadRGBASpan = i810ReadRGBASpan_555;
      ctx->Driver.ReadRGBAPixels = i810ReadRGBAPixels_555;
   }

   ctx->Driver.WriteCI8Span        =NULL;
   ctx->Driver.WriteCI32Span       =NULL;
   ctx->Driver.WriteMonoCISpan     =NULL;
   ctx->Driver.WriteCI32Pixels     =NULL;
   ctx->Driver.WriteMonoCIPixels   =NULL;
   ctx->Driver.ReadCI32Span        =NULL;
   ctx->Driver.ReadCI32Pixels      =NULL;
}
