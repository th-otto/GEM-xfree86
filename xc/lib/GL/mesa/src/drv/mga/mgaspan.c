#include "types.h"
#include "mgadd.h"
#include "mgalib.h"
#include "mgadma.h"
#include "mgalog.h"
#include "mgaspan.h"


#define LOCAL_VARS					\
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );		\
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;	\
   __DRIscreenPrivate *sPriv = mmesa->driScreen;	\
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;	\
   GLuint pitch = mgaScreen->backPitch;			\
   GLuint height = dPriv->h;				\
   char *buf = (char *)(sPriv->pFB +			\
			mmesa->drawOffset +		\
			dPriv->x * 2 +			\
			dPriv->y * pitch)

#define INIT_MONO_PIXEL(p) \
   GLushort p = MGA_CONTEXT( ctx )->MonoColor;

#define CLIPPIXEL(_x,_y) (_x >= minx && _x <= maxx && \
			  _y >= miny && _y <= maxy)


#define CLIPSPAN(_x,_y,_n,_x1,_n1)				\
	 if (_y < miny || _y >= maxy) _n1 = 0, _x1 = x;		\
         else {							\
            _n1 = _n;						\
	    _x1 = _x;						\
	    if (_x1 < minx) _n1 -= (minx - _x1), _x1 = minx;	\
	    if (_x1 + _n1 > maxx) n1 -= (_x1 + n1 - maxx);	\
         }





#define HW_CLIPLOOP()						\
  do {								\
    int _nc = mmesa->numClipRects;				\
    LOCK_HARDWARE_QUIESCENT(mmesa);				\
    while (_nc--) {						\
       int minx = mmesa->pClipRects[_nc].x1 - mmesa->drawX;		\
       int miny = mmesa->pClipRects[_nc].y1 - mmesa->drawY; 	\
       int maxx = mmesa->pClipRects[_nc].x2 - mmesa->drawX;		\
       int maxy = mmesa->pClipRects[_nc].y2 - mmesa->drawY;


#define HW_ENDCLIPLOOP()			\
    }						\
    UNLOCK_HARDWARE(mmesa);			\
  } while (0)


#define Y_FLIP(_y) (height - _y)
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( ((r & 0x1f) << 11) |	\
		                             ((g & 0x3f) << 5) |	\
		                             ((b & 0x1f)))
#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 11) & 0x1f;				\
   rgba[1] = (p >> 5) & 0x3f;				\
   rgba[2] = (p >> 0) & 0x1f;				\
   rgba[3] = 0;			/* or 255? */		\
} while(0)

#define TAG(x) mga##x##_565
#include "spantmp.h"





#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLushort *)(buf + _x*2 + _y*pitch)  = ( ((r & 0x1f) << 10) |	\
		                            ((g & 0x1f) << 5) |		\
                         		    ((b & 0x1f)))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLushort *)(buf + _x*2 + _y*pitch)  = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLushort p = *(GLushort *)(buf + _x*2 + _y*pitch);	\
   rgba[0] = (p >> 10) & 0x1f;				\
   rgba[1] = (p >> 5) & 0x1f;				\
   rgba[2] = (p >> 0) & 0x1f;				\
   rgba[3] = 0;			/* or 255? */		\
} while(0)

#define TAG(x) mga##x##_555
#include "spantmp.h"



#if 0
#define WRITE_RGBA( _x, _y, r, g, b, a )				\
   *(GLuint *)(buf + _x*4 + _y*pitch)  = ( ((r) << 16) |	\
		                           ((g) << 8) |		\
                         		   ((b)))

#define WRITE_PIXEL( _x, _y, p )  \
   *(GLuint *)(buf + _x*4 + _y*pitch)  = p

#define READ_RGBA( rgba, _x, _y )			\
do {							\
   GLuint p = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   rgba[0] = (p >> 16) & 0xff;				\
   rgba[1] = (p >> 8) & 0xff;				\
   rgba[2] = (p >> 0) & 0xff;				\
   rgba[3] = 0;			/* or 255? */		\
} while(0)

#define TAG(x) mga##x##_888
#include "spantmp.h"
#endif

void mgaDDInitSpanFuncs( GLcontext *ctx )
{
   if (1) {
      ctx->Driver.WriteRGBASpan = mgaWriteRGBASpan_565;
      ctx->Driver.WriteRGBSpan = mgaWriteRGBSpan_565;
      ctx->Driver.WriteMonoRGBASpan = mgaWriteMonoRGBASpan_565;
      ctx->Driver.WriteRGBAPixels = mgaWriteRGBAPixels_565;
      ctx->Driver.WriteMonoRGBAPixels = mgaWriteMonoRGBAPixels_565;
      ctx->Driver.ReadRGBASpan = mgaReadRGBASpan_565;
      ctx->Driver.ReadRGBAPixels = mgaReadRGBAPixels_565;
   } else {
      ctx->Driver.WriteRGBASpan = mgaWriteRGBASpan_555;
      ctx->Driver.WriteRGBSpan = mgaWriteRGBSpan_555;
      ctx->Driver.WriteMonoRGBASpan = mgaWriteMonoRGBASpan_555;
      ctx->Driver.WriteRGBAPixels = mgaWriteRGBAPixels_555;
      ctx->Driver.WriteMonoRGBAPixels = mgaWriteMonoRGBAPixels_555;
      ctx->Driver.ReadRGBASpan = mgaReadRGBASpan_555;
      ctx->Driver.ReadRGBAPixels = mgaReadRGBAPixels_555;
   }

   ctx->Driver.WriteCI8Span        =NULL;
   ctx->Driver.WriteCI32Span       =NULL;
   ctx->Driver.WriteMonoCISpan     =NULL;
   ctx->Driver.WriteCI32Pixels     =NULL;
   ctx->Driver.WriteMonoCIPixels   =NULL;
   ctx->Driver.ReadCI32Span        =NULL;
   ctx->Driver.ReadCI32Pixels      =NULL;
}
