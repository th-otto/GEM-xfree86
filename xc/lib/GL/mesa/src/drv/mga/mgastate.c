
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
#include "mgaregs.h"
#include "mga_drm_public.h"

static void mgaUpdateZMode(const GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int zmode = 0;

   if (ctx->Depth.Test) {
      switch(ctx->Depth.Func)  { 
      case GL_NEVER:
	 zmode = DC_zmode_nozcmp; break;
      case GL_ALWAYS:  
	 zmode = DC_zmode_nozcmp; break;
      case GL_LESS:
	 zmode = DC_zmode_zlt; break; 
      case GL_LEQUAL:
	 zmode = DC_zmode_zlte; break;
      case GL_EQUAL:
	 zmode = DC_zmode_ze; break;
      case GL_GREATER:
	 zmode = DC_zmode_zgt; break;
      case GL_GEQUAL:
	 zmode = DC_zmode_zgte; break;
      case GL_NOTEQUAL:
	 zmode = DC_zmode_zne; break;
      default:
	 break;
      }
      if (ctx->Depth.Mask)
	 zmode |= DC_atype_zi;
      else
	 zmode |= DC_atype_i;
   } else {
      zmode |= DC_zmode_nozcmp | DC_atype_i;
   }

   mmesa->Setup[MGA_CTXREG_DWGCTL] &= DC_zmode_MASK & DC_atype_MASK;
   mmesa->Setup[MGA_CTXREG_DWGCTL] |= zmode;
   mmesa->dirty |= MGA_UPLOAD_CTX;
}


static void mgaDDAlphaFunc(GLcontext *ctx, GLenum func, GLclampf ref)
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}


static void mgaDDBlendEquation(GLcontext *ctx, GLenum mode) 
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}

static void mgaDDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}

static void mgaDDBlendFuncSeparate( GLcontext *ctx, GLenum sfactorRGB, 
				    GLenum dfactorRGB, GLenum sfactorA,
				    GLenum dfactorA )
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
}



static void mgaDDLightModelfv(GLcontext *ctx, GLenum pname, 
			      const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
   }
}


static void mgaDDShadeModel(GLcontext *ctx, GLenum mode)
{
   if (1) {
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE; 
      mgaMsg(8, "mgaDDShadeModel: %x\n", mode);
   }
}


static void mgaDDDepthFunc(GLcontext *ctx, GLenum func)
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}

static void mgaDDDepthMask(GLcontext *ctx, GLboolean flag)
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
}




static void mgaUpdateFogAttrib( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mgaUI32 color = MGAPACKCOLOR888((mgaUI8)(ctx->Fog.Color[0]*255.0F), 
				   (mgaUI8)(ctx->Fog.Color[1]*255.0F), 
				   (mgaUI8)(ctx->Fog.Color[2]*255.0F));

   if (color != mmesa->Setup[MGA_CTXREG_FOGCOLOR]) 
      mmesa->Setup[MGA_CTXREG_FOGCOLOR] = color;

   mmesa->Setup[MGA_CTXREG_MACCESS] &= ~MA_fogen_enable;
   if (ctx->FogMode == FOG_FRAGMENT) 
      mmesa->Setup[MGA_CTXREG_MACCESS] |= MA_fogen_enable;

   mmesa->dirty |= MGA_UPLOAD_CTX;
}

static void mgaDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_FOG;
}




/* =============================================================
 * Alpha blending
 */


static void mgaUpdateAlphaMode(GLcontext *ctx)
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int a = 0;

   /* determine source of alpha for blending and testing */
   if ( !ctx->Texture.Enabled || (mmesa->Fallback & MGA_FALLBACK_TEXTURE)) 
      a |= AC_alphasel_diffused;
   else {
      switch (ctx->Texture.Unit[0].EnvMode) {
      case GL_DECAL:
      case GL_REPLACE:
	 a |= AC_alphasel_fromtex;
	 break;
      case GL_BLEND:
      case GL_MODULATE:
	 a |= AC_alphasel_modulated;
	 break;
      default:
      }
   }


   /* alpha test control - disabled by default.
    */
   if (ctx->Color.AlphaEnabled) {
      GLubyte ref = ctx->Color.AlphaRef;
      switch (ctx->Color.AlphaFunc) {
      case GL_NEVER:		
	 a |= AC_atmode_alt;
	 ref = 0;
	 break;
      case GL_LESS:
	 a |= AC_atmode_alt; 
	 break;
      case GL_GEQUAL:
	 a |= AC_atmode_agte; 
	 break;
      case GL_LEQUAL:  
	 a |= AC_atmode_alte; 
	 break;
      case GL_GREATER:  
	 a |= AC_atmode_agt; 
	 break;
      case GL_NOTEQUAL: 
	 a |= AC_atmode_ane; 
	 break;
      case GL_EQUAL: 
	 a |= AC_atmode_ae; 
	 break;
      case GL_ALWAYS:
	 a |= AC_atmode_noacmp; 
	 break;
      default:
      }
      a |= MGA_FIELD(AC_atref,ref);
   }

   /* blending control */
   if (ctx->Color.BlendEnabled) {
      switch (ctx->Color.BlendSrcRGB) {
      case GL_ZERO:
	 a |= AC_src_zero; break;
      case GL_SRC_ALPHA:
	 a |= AC_src_src_alpha; break;
      case GL_ONE:
	 a |= AC_src_one; break;
      case GL_DST_COLOR:
	 a |= AC_src_dst_color; break;
      case GL_ONE_MINUS_DST_COLOR:
	 a |= AC_src_om_dst_color; break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 a |= AC_src_om_src_alpha; break;
      case GL_DST_ALPHA:
	 if (0) /*(mgaScreen->Attrib & MGA_PF_HASALPHA)*/
	    a |= AC_src_dst_alpha;
	 else
	    a |= AC_src_one;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 if (0) /*(mgaScreen->Attrib & MGA_PF_HASALPHA)*/
	    a |= AC_src_om_dst_alpha; 
	 else 
	    a |= AC_src_zero;
	 break;
      case GL_SRC_ALPHA_SATURATE:
	 a |= AC_src_src_alpha_sat; 
	 break;
      default:		/* never happens */
      }

      switch (ctx->Color.BlendDstRGB) {
      case GL_SRC_ALPHA:
	 a |= AC_dst_src_alpha; break;
      case GL_ONE_MINUS_SRC_ALPHA:
	 a |= AC_dst_om_src_alpha; break;
      case GL_ZERO:
	 a |= AC_dst_zero; break;
      case GL_ONE:
	 a |= AC_dst_one; break;
      case GL_SRC_COLOR:
	 a |= AC_dst_src_color; break;
      case GL_ONE_MINUS_SRC_COLOR:
	 a |= AC_dst_om_src_color; break;
      case GL_DST_ALPHA:
	 if (0) /*(mgaDB->Attrib & MGA_PF_HASALPHA)*/
	    a |= AC_dst_dst_alpha;
	 else
	    a |= AC_dst_one;
	 break;
      case GL_ONE_MINUS_DST_ALPHA:
	 if (0) /*(mgaScreen->Attrib & MGA_PF_HASALPHA)*/
	    a |= AC_dst_om_dst_alpha;
	 else
	    a |= AC_dst_zero;
	 break;
      default:		/* never happens */
      }
   } else {
      a |= AC_src_one|AC_dst_zero;
   }

   mmesa->Setup[MGA_CTXREG_ALPHACTRL] = (AC_amode_alpha_channel | 
					AC_astipple_disable | 
					AC_aten_disable | 
					AC_atmode_noacmp | 
					a);
 
   mmesa->dirty |= MGA_UPLOAD_CTX;
}



/* =============================================================
 * Hardware clipping
 */

static void mgaUpdateClipping(const GLcontext *ctx)
{
#if 0
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;	
   int x1,x2,y1,y2;

   if ( ctx->Scissor.Enabled) {
      x1 = ctx->Scissor.X;
      x2 = ctx->Scissor.X + ctx->Scissor.Width - 1;
      y1 = dPriv->Height - ctx->Scissor.Y - ctx->Scissor.Height;
      y2 = dPriv->Height - ctx->Scissor.Y - 1;
   } else {
      x1 = 0;
      y1 = 0;
      x2 = mgaDB->Width-1;
      y2 = mgaDB->Height-1;
   }
  
   if (x1 < 0) x1 = 0;
   if (y1 < 0) y1 = 0;
   if (x2 >= mgaDB->Width) x2 = mgaDB->Width-1;
   if (y2 >= mgaDB->Height) y2 = mgaDB->Height-1;

   if (x1 > x2 || y1 > y2) {
      x1 = 0; x2 = 0;
      y2 = 0; y1 = 1;
   }


   mmesa->Setup[MGA_CTXREG_CXBNDRY] = (MGA_FIELD(CXB_cxright,x2) | 
				      MGA_FIELD(CXB_cxleft,x1));
   mmesa->Setup[MGA_CTXREG_YTOP] = y1*mgaDB->Pitch;
   mmesa->Setup[MGA_CTXREG_YBOT] = y2*mgaDB->Pitch;


   mmesa->dirty |= MGA_UPLOAD_CTX;
#endif
}


static void mgaDDScissor( GLcontext *ctx, GLint x, GLint y, 
			  GLsizei w, GLsizei h )
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CLIP;
}


/* ======================================================================
 * New stuff for DRI state.
 */

static void mgaDDDither(GLcontext *ctx, GLboolean enable)
{
}


static GLboolean mgaDDSetBuffer(GLcontext *ctx, GLenum mode )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mmesa->Fallback &= ~MGA_FALLBACK_BUFFER;

   if (mode == GL_FRONT_LEFT) 
   {
      mmesa->drawOffset = mmesa->mgaScreen->fbOffset;
      mmesa->Setup[MGA_CTXREG_DSTORG] = mmesa->mgaScreen->fbOffset;
      mmesa->dirty |= MGA_UPLOAD_CTX;
      mgaXMesaSetFrontClipRects( mmesa );
      return GL_TRUE;
   } 
   else if (mode == GL_BACK_LEFT) 
   {
      mmesa->drawOffset = mmesa->mgaScreen->backOffset;
      mmesa->Setup[MGA_CTXREG_DSTORG] = mmesa->mgaScreen->backOffset;
      mmesa->dirty |= MGA_UPLOAD_CTX;
      mgaXMesaSetBackClipRects( mmesa );
      return GL_TRUE;
   }
   else 
   {
      mmesa->Fallback |= MGA_FALLBACK_BUFFER;
      return GL_FALSE;
   }
}



static void mgaDDSetColor(GLcontext *ctx, 
			   GLubyte r, GLubyte g,
			   GLubyte b, GLubyte a )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mmesa->MonoColor = mgaPackColor( mmesa->mgaScreen->fbFormat,
				      r, g, b, a );
}


static void mgaDDClearColor(GLcontext *ctx, 
			     GLubyte r, GLubyte g,
			     GLubyte b, GLubyte a )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   mmesa->ClearColor = mgaPackColor( mmesa->mgaScreen->fbFormat,
				      r, g, b, a );
}


/* =============================================================
 * Culling
 */

#define _CULL_DISABLE 0
#define _CULL_NEGATIVE ((1<<11)|(1<<5)|(1<<16))
#define _CULL_POSITIVE (1<<11)


static void mgaUpdateCull( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint mode = _CULL_DISABLE;
   
   if (ctx->Polygon.CullFlag && 
       ctx->PB->primitive == GL_POLYGON &&       
       ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) 
   {
      mode = _CULL_NEGATIVE;
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE);
      if (ctx->Polygon.FrontFace != GL_CCW)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE);
      if (mmesa->multitex)
	 mode ^= (_CULL_POSITIVE ^ _CULL_NEGATIVE); /* why??? */
   }

   mmesa->Setup[MGA_CTXREG_WFLAG] = mode;
   mmesa->dirty |= MGA_UPLOAD_CTX;
}


static void mgaDDCullFaceFrontFace(GLcontext *ctx, GLenum mode)
{
   MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CULL;
}




/* =============================================================
 * Color masks
 */

/* Mesa calls this from the wrong place:
 */
static GLboolean mgaDDColorMask(GLcontext *ctx, 
				GLboolean r, GLboolean g, 
				GLboolean b, GLboolean a )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaScreenPrivate *mgaScreen = mmesa->mgaScreen;	


   GLuint mask = mgaPackColor(mgaScreen->Attrib,
			      ctx->Color.ColorMask[RCOMP],
			      ctx->Color.ColorMask[GCOMP],
			      ctx->Color.ColorMask[BCOMP],
			      ctx->Color.ColorMask[ACOMP]);
  
   switch (mgaScreen->Attrib & MGA_PF_MASK)
   {
   case MGA_PF_555:
   case MGA_PF_565:
      mask = mask | (mask << 16);
      break;
   }

   if (mmesa->Setup[MGA_CTXREG_PLNWT] != mask) {
      mmesa->Setup[MGA_CTXREG_PLNWT] = mask;      
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_MASK; 
      mmesa->dirty |= MGA_UPLOAD_CTX;
   }

   return 1;
}

/* =============================================================
 */




static void mgaDDPrintDirty( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s\n",	   
	   msg,
	   (unsigned int) state,
	   (state & MGA_REQUIRE_QUIESCENT) ? "req-quiescent, " : "",
	   (state & MGA_UPLOAD_TEX0IMAGE)  ? "upload-tex0-img, " : "",
	   (state & MGA_UPLOAD_TEX1IMAGE)  ? "upload-tex1-img, " : "",
	   (state & MGA_UPLOAD_CTX)        ? "upload-ctx, " : "",
	   (state & MGA_UPLOAD_TEX0)       ? "upload-tex0, " : "",
	   (state & MGA_UPLOAD_TEX1)       ? "upload-tex1, " : ""
	   );
}


/* Push the state into the sarea and/or texture memory.
 */
void mgaEmitHwStateLocked( mgaContextPtr mmesa )
{
   if (MGA_DEBUG & MGA_DEBUG_VERBOSE_MSG)
      mgaDDPrintDirty( "mgaEmitHwStateLocked", mmesa->dirty );

   if ((mmesa->dirty & MGA_UPLOAD_TEX0IMAGE) && mmesa->CurrentTexObj[0])
      mgaUploadTexImages(mmesa, mmesa->CurrentTexObj[0]);
   
   if ((mmesa->dirty & MGA_UPLOAD_TEX1IMAGE) && mmesa->CurrentTexObj[1])
      mgaUploadTexImages(mmesa, mmesa->CurrentTexObj[1]);
  
   if (mmesa->dirty & MGA_UPLOAD_CTX) {
      memcpy( mmesa->sarea->ContextState, 
	      mmesa->Setup,
	      sizeof(mmesa->Setup));

      if (mmesa->CurrentTexObj[0])
	 memcpy(mmesa->sarea->TexState[0],
		mmesa->CurrentTexObj[0]->Setup,
		sizeof(mmesa->sarea->TexState[0]));

      if (mmesa->CurrentTexObj[1])
	 memcpy(mmesa->sarea->TexState[1],
		mmesa->CurrentTexObj[1]->Setup,
		sizeof(mmesa->sarea->TexState[1]));
   }

   if (mmesa->dirty & MGA_UPLOAD_PIPE) 
      mmesa->sarea->WarpPipe = mmesa->setupindex & MGA_WARP_T2GZSAF;
      
   mmesa->sarea->dirty |= mmesa->dirty;
   mmesa->dirty = 0;
}



/* =============================================================
 */

static void mgaDDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   switch(cap) {
   case GL_ALPHA_TEST:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
      break;
   case GL_BLEND:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_ALPHA;
      break;
   case GL_DEPTH_TEST:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_DEPTH;
      break;
   case GL_SCISSOR_TEST:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CLIP;
      break;
   case GL_FOG:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_FOG;
      break;
   case GL_CULL_FACE:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_CULL;
      break;
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
      break;
   default:
      ; 
   }    
}


/* =============================================================
 */

/* Just need to note that it has changed - the kernel will do the
 * upload the next time we fire a dma buffer.
 */
static void mgaWarpUpdateState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int index = mmesa->setupindex;
 
   index &= ~(MGA_WIN_BIT|MGA_TEX0_BIT|MGA_RGBA_BIT);
   index |= MGA_ALPHA_BIT;

   if (index != mmesa->warp_pipe)
   {
      mmesa->warp_pipe = index;
      mmesa->new_state |= MGA_NEW_WARP;
   }
}



/* =============================================================
 */

static void mgaDDPrintState( const char *msg, GLuint state )
{
   mgaMsg(1, "%s (0x%x): %s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & MGA_NEW_DEPTH)   ? "depth, " : "",
	   (state & MGA_NEW_ALPHA)   ? "alpha, " : "",
	   (state & MGA_NEW_FOG)     ? "fog, " : "",
	   (state & MGA_NEW_CLIP)    ? "clip, " : "",
	   (state & MGA_NEW_MASK)    ? "colormask, " : "",
	   (state & MGA_NEW_CULL)    ? "cull, " : "",
	   (state & MGA_NEW_TEXTURE) ? "texture, " : "",
	   (state & MGA_NEW_CONTEXT) ? "context, " : "");
}

void mgaDDUpdateHwState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   int new_state = mmesa->new_state;

   if (new_state) 
   {
      mmesa->new_state = 0;

      /* Emit any vertices for the current state.  This will also
       * push the current state into the sarea.
       */
/*        mgaFlushVertices( mmesa ); */

      if (MESA_VERBOSE&VERBOSE_DRIVER)
	 mgaDDPrintState("UpdateHwState", new_state);

      if (new_state & MGA_NEW_DEPTH)
      {
	 mgaUpdateZMode(ctx);
	 mgaDDInitDepthFuncs(ctx);
      }

      if (new_state & MGA_NEW_ALPHA)
	 mgaUpdateAlphaMode(ctx);

      if (new_state & MGA_NEW_FOG)
	 mgaUpdateFogAttrib(ctx);

      if (new_state & MGA_NEW_CLIP)
	 mgaUpdateClipping(ctx);

      if (new_state & (MGA_NEW_WARP|MGA_NEW_CULL))
	 mgaUpdateCull(ctx);

      if (new_state & (MGA_NEW_WARP|MGA_NEW_TEXTURE))
	 mgaUpdateTextureState(ctx);

      mmesa->new_state = 0; /* tex uploads scribble newstate */
   }
}







void mgaDDReducedPrimitiveChange( GLcontext *ctx, GLenum prim )
{
   mgaFlushVertices( MGA_CONTEXT(ctx) );
   mgaUpdateCull(ctx);
}


#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|\
                      NEW_TEXTURE_MATRIX|\
                      NEW_USER_CLIP|NEW_CLIENT_STATE|\
                      NEW_TEXTURE_ENABLE))

void mgaDDUpdateState( GLcontext *ctx )
{
   mgaContextPtr mmesa = MGA_CONTEXT( ctx );
   mgaglx.c_setupPointers++;
   
   if (ctx->NewState & INTERESTED) {
      mgaDDChooseRenderState(ctx);  
      mgaChooseRasterSetupFunc(ctx);
      mgaWarpUpdateState(ctx);
   }     

   /* Have to do this here to detect texture fallbacks in time:
    */
   if (MGA_CONTEXT(ctx)->new_state & MGA_NEW_TEXTURE)
      mgaDDUpdateHwState( ctx );

   if (!mmesa->Fallback || mgaglx.noFallback) {
      ctx->Driver.PointsFunc=mmesa->PointsFunc;
      ctx->Driver.LineFunc=mmesa->LineFunc;
      ctx->Driver.TriangleFunc=mmesa->TriangleFunc;
      ctx->Driver.QuadFunc=mmesa->QuadFunc;
   }
}


void mgaDDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState = mgaDDUpdateState;
   ctx->Driver.Enable = mgaDDEnable;
   ctx->Driver.LightModelfv = mgaDDLightModelfv;
   ctx->Driver.AlphaFunc = mgaDDAlphaFunc;
   ctx->Driver.BlendEquation = mgaDDBlendEquation;
   ctx->Driver.BlendFunc = mgaDDBlendFunc;
   ctx->Driver.BlendFuncSeparate = mgaDDBlendFuncSeparate;
   ctx->Driver.DepthFunc = mgaDDDepthFunc;
   ctx->Driver.DepthMask = mgaDDDepthMask;
   ctx->Driver.Fogfv = mgaDDFogfv;
   ctx->Driver.Scissor = mgaDDScissor;
   ctx->Driver.ShadeModel = mgaDDShadeModel;
   ctx->Driver.CullFace = mgaDDCullFaceFrontFace;
   ctx->Driver.FrontFace = mgaDDCullFaceFrontFace;
   ctx->Driver.ColorMask = mgaDDColorMask;
   ctx->Driver.ReducedPrimitiveChange = mgaDDReducedPrimitiveChange;
   ctx->Driver.RenderStart = mgaDDUpdateHwState; 
   ctx->Driver.RenderFinish = 0; 

   ctx->Driver.SetBuffer = mgaDDSetBuffer;
   ctx->Driver.Color = mgaDDSetColor;
   ctx->Driver.ClearColor = mgaDDClearColor;
   ctx->Driver.Dither = mgaDDDither;

   ctx->Driver.Index = 0;
   ctx->Driver.ClearIndex = 0;
   ctx->Driver.IndexMask = 0;

}

