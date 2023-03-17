
#include <stdio.h>

#include "types.h"
#include "enums.h"
#include "pb.h"
#include "dd.h"

#include "mm.h"
#include "i810lib.h"
#include "i810dd.h"
#include "i810context.h"
#include "i810state.h"
#include "i810depth.h"
#include "i810tex.h"
#include "i810log.h"
#include "i810vb.h"
#include "i810tris.h"



static void i810DDAlphaFunc(GLcontext *ctx, GLenum func, GLclampf ref)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   CARD32 a = (ZA_UPDATE_ALPHAFUNC|ZA_UPDATE_ALPHAREF);

   switch (ctx->Color.AlphaFunc) {
   case GL_NEVER:    a |= ZA_ALPHA_NEVER;    break;
   case GL_LESS:     a |= ZA_ALPHA_LESS;     break;
   case GL_GEQUAL:   a |= ZA_ALPHA_GEQUAL;   break;
   case GL_LEQUAL:   a |= ZA_ALPHA_LEQUAL;   break;
   case GL_GREATER:  a |= ZA_ALPHA_GREATER;  break;
   case GL_NOTEQUAL: a |= ZA_ALPHA_NOTEQUAL; break;
   case GL_EQUAL:    a |= ZA_ALPHA_EQUAL;    break;
   case GL_ALWAYS:   a |= ZA_ALPHA_ALWAYS;   break;
   default: return;
   }

   a |= ctx->Color.AlphaRef << ZA_ALPHAREF_SHIFT;

   imesa->dirty |= I810_UPLOAD_CTX;
   imesa->Setup[I810_CTXREG_ZA] &= ~(ZA_ALPHA_MASK|ZA_ALPHAREF_MASK);
   imesa->Setup[I810_CTXREG_ZA] |= a;
}

/* This shouldn't get called, as the extension is disabled.  However,
 * there are internal Mesa calls, and rogue use of the api which must be
 * caught.
 */
static void i810DDBlendEquation(GLcontext *ctx, GLenum mode) 
{
   if (mode != GL_FUNC_ADD_EXT) {
      ctx->Color.BlendEquation = GL_FUNC_ADD_EXT;
      i810Error("Unsupported blend equation");
      exit(1);
   }
}

static void i810DDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint a;

   a = SDM_UPDATE_SRC_BLEND | SDM_UPDATE_DST_BLEND;

   switch (ctx->Color.BlendSrcRGB) {
   case GL_ZERO:                a |= SDM_SRC_ZERO; break;
   case GL_SRC_ALPHA:           a |= SDM_SRC_SRC_ALPHA; break;
   case GL_ONE:                 a |= SDM_SRC_ONE; break;
   case GL_DST_COLOR:           a |= SDM_SRC_DST_COLOR; break;
   case GL_ONE_MINUS_DST_COLOR: a |= SDM_SRC_INV_DST_COLOR; break;
   case GL_ONE_MINUS_SRC_ALPHA: a |= SDM_SRC_INV_SRC_ALPHA; break;
   case GL_DST_ALPHA:           a |= SDM_SRC_ONE; break;
   case GL_ONE_MINUS_DST_ALPHA: a |= SDM_SRC_ZERO; break;
   case GL_SRC_ALPHA_SATURATE:      
      a |= SDM_SRC_SRC_ALPHA;	/* use GFXRENDERSTATE_COLOR_FACTOR ??? */
      break;
   default: 
      i810Error("unknown blend source func");
      exit(1);
      return;
   }

   switch (ctx->Color.BlendDstRGB) {
   case GL_SRC_ALPHA:           a |= SDM_DST_SRC_ALPHA; break;
   case GL_ONE_MINUS_SRC_ALPHA: a |= SDM_DST_INV_SRC_ALPHA; break;
   case GL_ZERO:                a |= SDM_DST_ZERO; break;
   case GL_ONE:                 a |= SDM_DST_ONE; break;
   case GL_SRC_COLOR:           a |= SDM_DST_SRC_COLOR; break;
   case GL_ONE_MINUS_SRC_COLOR: a |= SDM_DST_INV_SRC_COLOR; break;
   case GL_DST_ALPHA:           a |= SDM_DST_ONE; break;
   case GL_ONE_MINUS_DST_ALPHA: a |= SDM_DST_ZERO; break;
   default: 
      i810Error( "unknown blend dst func");
      exit(1);
      return;      
   }  

   imesa->dirty |= I810_UPLOAD_CTX;
   imesa->Setup[I810_CTXREG_SDM] &= ~(SDM_SRC_MASK|SDM_DST_MASK);
   imesa->Setup[I810_CTXREG_SDM] |= a;
}


/* Shouldn't be called as the extension is disabled.
 */
static void i810DDBlendFuncSeparate( GLcontext *ctx, GLenum sfactorRGB, 
				     GLenum dfactorRGB, GLenum sfactorA,
				     GLenum dfactorA )
{
   if (dfactorRGB != dfactorA || sfactorRGB != sfactorA) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBlendEquation (disabled)");
   }

   i810DDBlendFunc( ctx, sfactorRGB, dfactorRGB );
}



static void i810DDDepthFunc(GLcontext *ctx, GLenum func)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   int zmode;

   switch(func)  { 
   case GL_NEVER: zmode = LCS_Z_NEVER; break;
   case GL_ALWAYS: zmode = LCS_Z_ALWAYS; break;
   case GL_LESS: zmode = LCS_Z_LESS; break; 
   case GL_LEQUAL: zmode = LCS_Z_LEQUAL; break;
   case GL_EQUAL: zmode = LCS_Z_EQUAL; break;
   case GL_GREATER: zmode = LCS_Z_GREATER; break;
   case GL_GEQUAL: zmode = LCS_Z_GEQUAL; break;
   case GL_NOTEQUAL: zmode = LCS_Z_NOTEQUAL; break;
   default: return;
   }

   imesa->Setup[I810_CTXREG_LCS] &= ~LCS_Z_MASK;
   imesa->Setup[I810_CTXREG_LCS] |= LCS_UPDATE_ZMODE | zmode;
   imesa->dirty |= I810_UPLOAD_CTX;
}

static void i810DDDepthMask(GLcontext *ctx, GLboolean flag)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   imesa->dirty |= I810_UPLOAD_CTX;
   imesa->Setup[I810_CTXREG_B2] &= ~B2_ZB_WRITE_ENABLE;
   imesa->Setup[I810_CTXREG_B2] |= B2_UPDATE_ZB_WRITE_ENABLE;

   if (flag)
     imesa->Setup[I810_CTXREG_B2] |= B2_ZB_WRITE_ENABLE;
}










/* =============================================================
 * Hardware clipping
 */


static void i810DDScissor( GLcontext *ctx, GLint x, GLint y, 
			  GLsizei w, GLsizei h )
{ 
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   int x1,x2,y1,y2;

   x1 = ctx->Scissor.X;
   x2 = ctx->Scissor.X + ctx->Scissor.Width - 1;
   y1 = dPriv->h - ctx->Scissor.Y - ctx->Scissor.Height;
   y2 = dPriv->h - ctx->Scissor.Y - 1;
  
   if (x1 < 0) x1 = 0;
   if (y1 < 0) y1 = 0;
   if (x2 >= dPriv->w) x2 = dPriv->w-1;
   if (y2 >= dPriv->h) y2 = dPriv->h-1;

   if (x1 > x2 || y1 > y2) {
      x1 = 0; x2 = 0;
      y2 = 0; y1 = 1;
   }

   /* Need to push this into drawing rectangle.
    */
#if 0
   imesa->Setup[I810_CTXREG_SCI0] = GFX_OP_SCISSOR_INFO;
   imesa->Setup[I810_CTXREG_SCI1] = (y1<<16)|x1;
   imesa->Setup[I810_CTXREG_SCI2] = (y2<<16)|x2;


   /* Need to intersect with cliprects???
    */
   imesa->dirty |= I810_UPLOAD_CTX;
#endif

}


static void i810DDDither(GLcontext *ctx, GLboolean enable)
{
}


static GLboolean i810DDSetBuffer(GLcontext *ctx, GLenum mode )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);


   fprintf(stderr, "i810DDSetBuffer %s\n", gl_lookup_enum_by_nr( mode ));

   imesa->Fallback &= ~I810_FALLBACK_BUFFER;

   if (mode == GL_FRONT_LEFT) 
   {
      imesa->drawOffset = imesa->i810Screen->fbOffset;
      imesa->BufferSetup[I810_DESTREG_DI1] = (imesa->i810Screen->fbOffset | 
					      imesa->i810Screen->auxPitchBits);
      imesa->dirty |= I810_UPLOAD_BUFFERS;
      i810XMesaSetFrontClipRects( imesa );
      return GL_TRUE;
   } 
   else if (mode == GL_BACK_LEFT) 
   {
      imesa->drawOffset = imesa->i810Screen->backOffset;
      imesa->BufferSetup[I810_DESTREG_DI1] = (imesa->i810Screen->backOffset | 
					      imesa->i810Screen->auxPitchBits);
      imesa->dirty |= I810_UPLOAD_BUFFERS;
      i810XMesaSetBackClipRects( imesa );
      return GL_TRUE;
   }

   imesa->Fallback |= I810_FALLBACK_BUFFER;
   return GL_FALSE;
}



static void i810DDSetColor(GLcontext *ctx, 
			   GLubyte r, GLubyte g,
			   GLubyte b, GLubyte a )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   imesa->MonoColor = i810PackColor( imesa->i810Screen->fbFormat,
				      r, g, b, a );
}


static void i810DDClearColor(GLcontext *ctx, 
			     GLubyte r, GLubyte g,
			     GLubyte b, GLubyte a )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   imesa->ClearColor = i810PackColor( imesa->i810Screen->fbFormat,
				      r, g, b, a );
}


/* =============================================================
 * Culling - the i810 isn't quite as clean here as the rest of
 *           its interfaces, but it's not bad.
 */
static void i810DDCullFaceFrontFace(GLcontext *ctx, GLenum unused)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   GLuint mode = LCS_CULL_BOTH;
   
   if (ctx->Polygon.CullFaceMode != GL_FRONT_AND_BACK) {
      mode = LCS_CULL_CW;
      if (ctx->Polygon.CullFaceMode == GL_FRONT)
	 mode ^= (LCS_CULL_CW ^ LCS_CULL_CCW);
      if (ctx->Polygon.FrontFace != GL_CCW)
	 mode ^= (LCS_CULL_CW ^ LCS_CULL_CCW);
   }

   imesa->LcsCullMode = mode;

   if (ctx->Polygon.CullFlag && ctx->PB->primitive == GL_POLYGON)
   {
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_CULL_MASK;
      imesa->Setup[I810_CTXREG_LCS] |= (LCS_UPDATE_CULL_MODE | mode);
   }
}


static void i810DDReducedPrimitiveChange( GLcontext *ctx, GLenum prim )
{
   if (ctx->Polygon.CullFlag) {
      i810ContextPtr imesa = I810_CONTEXT(ctx);
      GLuint mode = imesa->LcsCullMode;

      if (ctx->PB->primitive != GL_POLYGON)
	 mode = LCS_CULL_DISABLE;

      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_LCS] &= ~LCS_CULL_MASK;
      imesa->Setup[I810_CTXREG_LCS] |= (LCS_UPDATE_CULL_MODE | mode);

      LOCK_HARDWARE(imesa);
      i810EmitHwStateLocked( imesa );	
      UNLOCK_HARDWARE(imesa);
   }
}



/* =============================================================
 * Color masks
 */

/* Mesa calls this from the wrong place.
 *
 * Its a fallback...
 */
static GLboolean i810DDColorMask(GLcontext *ctx, 
				 GLboolean r, GLboolean g, 
				 GLboolean b, GLboolean a )
{
   return 1;
}

/* Seperate specular not fully implemented in hardware...  Needs
 * some interaction with material state?  Just punt to software
 * in all cases?
 */
static void i810DDLightModelfv(GLcontext *ctx, GLenum pname, 
			      const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      I810_CONTEXT(ctx)->new_state |= I810_NEW_TEXTURE;
   }
}



/* =============================================================
 * Fog
 */

static void i810DDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   if (pname == GL_FOG_COLOR) {
      GLuint color = (((GLubyte)(ctx->Fog.Color[0]*255.0F) << 16) |
		      ((GLubyte)(ctx->Fog.Color[1]*255.0F) << 8) |
		      ((GLubyte)(ctx->Fog.Color[2]*255.0F) << 0));

      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_FOG] = ((GFX_OP_FOG_COLOR | color) &
				      ~FOG_RESERVED_MASK);
   }
}


/* =============================================================
 */

static void i810DDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   switch(cap) {
   case GL_ALPHA_TEST:
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_B1] &= ~B1_ALPHA_TEST_ENABLE;
      imesa->Setup[I810_CTXREG_B1] |= B1_UPDATE_ALPHA_TEST_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_ALPHA_TEST_ENABLE;
      break;
   case GL_BLEND:
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_B1] &= ~B1_BLEND_ENABLE;
      imesa->Setup[I810_CTXREG_B1] |= B1_UPDATE_BLEND_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_BLEND_ENABLE;
      break;
   case GL_DEPTH_TEST:
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_B1] &= ~B1_Z_TEST_ENABLE;
      imesa->Setup[I810_CTXREG_B1] |= B1_UPDATE_Z_TEST_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_Z_TEST_ENABLE;
      break;
   case GL_SCISSOR_TEST:
#if 0
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_SC] &= ~SC_ENABLE_MASK;
      imesa->Setup[I810_CTXREG_SC] |= SC_UPDATE_SCISSOR;
      if (state)
	 imesa->Setup[I810_CTXREG_SC] |= SC_ENABLE;
#endif
      break;
   case GL_FOG:
      imesa->dirty |= I810_UPLOAD_CTX;
      imesa->Setup[I810_CTXREG_B1] &= ~B1_FOG_ENABLE;
      imesa->Setup[I810_CTXREG_B1] |= B1_UPDATE_FOG_ENABLE;
      if (state)
	 imesa->Setup[I810_CTXREG_B1] |= B1_FOG_ENABLE;
      break;
   case GL_CULL_FACE:
      if (ctx->PB->primitive == GL_POLYGON) {
	 imesa->dirty |= I810_UPLOAD_CTX;
	 imesa->Setup[I810_CTXREG_LCS] &= ~LCS_CULL_MASK;
	 imesa->Setup[I810_CTXREG_LCS] |= LCS_UPDATE_CULL_MODE;
	 if (state)
	    imesa->Setup[I810_CTXREG_LCS] |= imesa->LcsCullMode;
	 else
	    imesa->Setup[I810_CTXREG_LCS] |= LCS_CULL_DISABLE;
      }
      break;
   case GL_TEXTURE_2D:      
      imesa->new_state |= I810_NEW_TEXTURE;
      imesa->dirty |= I810_UPLOAD_CTX;
      if (ctx->Texture.CurrentUnit == 0) {
	 imesa->Setup[I810_CTXREG_MT] &= ~MT_TEXEL0_ENABLE;
	 imesa->Setup[I810_CTXREG_MT] |= MT_UPDATE_TEXEL0_STATE;
	 if (state)
	    imesa->Setup[I810_CTXREG_MT] |= MT_TEXEL0_ENABLE;
      } else {
	 imesa->Setup[I810_CTXREG_MT] &= ~MT_TEXEL1_ENABLE;
	 imesa->Setup[I810_CTXREG_MT] |= MT_UPDATE_TEXEL1_STATE;
	 if (state)
	    imesa->Setup[I810_CTXREG_MT] |= MT_TEXEL1_ENABLE;
      }
      break;
   default:
      ; 
   }    
}



/* =============================================================
 */

/* Delightfully few possibilities:
 */
void i810DDPrintState( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s\n",
	   msg,
	   (unsigned int) state,
	   (state & I810_NEW_TEXTURE) ? "texture, " : "");
}

void i810DDUpdateHwState( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   if (imesa->new_state & I810_NEW_TEXTURE)
      i810UpdateTextureState( ctx );

   imesa->new_state = 0;

   if (imesa->dirty) {
      LOCK_HARDWARE(imesa);
      i810EmitHwStateLocked( imesa );	
      UNLOCK_HARDWARE(imesa);
   }
}



/*
 * i810DmaExecute
 * Add a block of data to the dma buffer
 *
 * -- In ring buffer mode, must be called with lock held, and translates to 
 *    OUT_RING rather than outbatch
 *
 * -- In dma mode, probably won't be used because state updates will have
 *    to be done via the kernel for security...
 */
static void i810DmaExecute( i810ContextPtr imesa,
			    GLuint *code, int dwords, const char *where ) 
{	
   if (I810_DEBUG & DEBUG_VERBOSE_RING)
      fprintf(stderr, "i810DmaExecute (%s)\n", where);

   if (dwords & 1) {
      i810Error( "Misaligned buffer in i810DmaExecute\n" );
      exit(1);
   }

   {
      int i;
      BEGIN_BATCH( imesa, dwords);

      for ( i = 0 ; i < dwords ; i++ ) {
	 if (0) fprintf(stderr, "%d: %x\n", i, code[i]);
	 OUT_BATCH( code[i] );
      }

      ADVANCE_BATCH();
   }
}



void i810EmitScissorValues( i810ContextPtr imesa, int nr, int emit )
{
   int x1 = imesa->pClipRects[nr].x1 - imesa->drawX;
   int y1 = imesa->pClipRects[nr].y1 - imesa->drawY;
   int x2 = imesa->pClipRects[nr].x2 - imesa->drawX;
   int y2 = imesa->pClipRects[nr].y2 - imesa->drawY;

   if (I810_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "i810EmitScissorValues %d,%d - %d,%d\n",
	      x1,y1,x2,y2);

   imesa->ClipSetup[I810_CLIPREG_SCI1] = (x1 | (y1 << 16));
   imesa->ClipSetup[I810_CLIPREG_SCI2] = (x2 | (y2 << 16));
   imesa->ClipSetup[I810_CLIPREG_SC] = ( GFX_OP_SCISSOR |
					 SC_UPDATE_SCISSOR |
					 SC_ENABLE );

   if (emit)
      i810DmaExecute( imesa, 
		      imesa->ClipSetup, I810_CLIP_SETUP_SIZE, "cliprect" );
}

void i810EmitDrawingRectangle( i810ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   i810ScreenPrivate *i810Screen = imesa->i810Screen;

   int x0 = imesa->drawX;
   int y0 = imesa->drawY;
   int x1 = x0 + dPriv->w;
   int y1 = y0 + dPriv->h;


   /* Coordinate origin of the window - may be offscreen.
    */
   imesa->BufferSetup[I810_DESTREG_DR4] = ((y0<<16) | 
					   (((unsigned)x0)&0xFFFF));

  
   /* Clip to screen.
    */
   if (x0 < 0) x0 = 0;
   if (y0 < 0) y0 = 0;
   if (x1 > i810Screen->width-1) x1 = i810Screen->width-1;
   if (y1 > i810Screen->height-1) y1 = i810Screen->height-1;


   /* Onscreen drawing rectangle.
    *
    * TODO: clip again to GL scissor values.
    */
   imesa->BufferSetup[I810_DESTREG_DR2] = ((y0<<16) | x0);
   imesa->BufferSetup[I810_DESTREG_DR3] = ((y1<<16) | x1);
   imesa->dirty |= I810_UPLOAD_BUFFERS;
}


static void i810DDPrintDirty( const char *msg, GLuint state )
{
   fprintf(stderr, "%s (0x%x): %s%s%s%s%s%s%s\n",	   
	   msg,
	   (unsigned int) state,
	   (state & I810_REFRESH_RING)      ? "read-lp-ring, " : "",
	   (state & I810_REQUIRE_QUIESCENT) ? "req-quiescent, " : "",
	   (state & I810_UPLOAD_TEX0IMAGE)  ? "upload-tex0, " : "",
	   (state & I810_UPLOAD_TEX1IMAGE)  ? "upload-tex1, " : "",
	   (state & I810_UPLOAD_CTX)        ? "upload-ctx, " : "",
	   (state & I810_UPLOAD_BUFFERS)    ? "upload-bufs, " : "",
	   (state & I810_EMIT_CLIPRECT)     ? "emit-single-cliprect, " : ""
	   );
}


/* Spew the state onto the ringbuffer and/or texture memory.
 */
void i810EmitHwStateLocked( i810ContextPtr imesa )
{
   if (I810_DEBUG & DEBUG_VERBOSE_API)
      i810DDPrintDirty( "i810EmitHwStateLocked", imesa->dirty );

   if (imesa->dirty & I810_REQUIRE_QUIESCENT) {
      i810glx.c_drawWaits += _I810Sync( imesa );
   }

   /* TODO: Refine mechanism to be equivalent to UploadSubImage
    */
   if ((imesa->dirty & I810_UPLOAD_TEX0IMAGE) && imesa->CurrentTexObj[0])
      i810UploadTexImages(imesa, imesa->CurrentTexObj[0]);
   
   if ((imesa->dirty & I810_UPLOAD_TEX1IMAGE) && imesa->CurrentTexObj[1])
      i810UploadTexImages(imesa, imesa->CurrentTexObj[1]);
  
   if ((imesa->dirty & I810_UPLOAD_CTX)) {
      i810DmaExecute( imesa, imesa->Setup, I810_CTX_SETUP_SIZE, "context" );

      if (imesa->CurrentTexObj[0])
	 i810DmaExecute( imesa, imesa->CurrentTexObj[0]->Setup,
			 I810_TEX_SETUP_SIZE, "tex-0");

      if (imesa->CurrentTexObj[1]) 
	 i810DmaExecute( imesa, imesa->CurrentTexObj[1]->Setup,
			 I810_TEX_SETUP_SIZE, "tex-1");
   }

   if (imesa->dirty & I810_UPLOAD_BUFFERS) 
      i810DmaExecute( imesa, imesa->BufferSetup, 
		      I810_DEST_SETUP_SIZE, "buffers" );
   
   if (imesa->dirty & I810_EMIT_CLIPRECT)
      i810DmaExecute( imesa, imesa->ClipSetup, 
		      I810_CLIP_SETUP_SIZE, "cliprect" );

   imesa->dirty = 0;
}


void i810DDInitState( i810ContextPtr imesa )
{
   i810ScreenPrivate *i810Screen = imesa->i810Screen;

   memset(imesa->Setup, 0, sizeof(imesa->Setup));

   imesa->Setup[I810_CTXREG_VF] = (GFX_OP_VERTEX_FMT |
				   VF_TEXCOORD_COUNT_2 |
				   VF_SPEC_FOG_ENABLE |
				   VF_RGBA_ENABLE |
				   VF_XYZW);

   imesa->Setup[I810_CTXREG_MT] = (GFX_OP_MAP_TEXELS |
				   MT_UPDATE_TEXEL1_STATE |
				   MT_TEXEL1_COORD1 |
				   MT_TEXEL1_MAP1 |
				   MT_TEXEL1_DISABLE |
				   MT_UPDATE_TEXEL0_STATE |
				   MT_TEXEL0_COORD0 |
				   MT_TEXEL0_MAP0 |
				   MT_TEXEL0_DISABLE);

   imesa->Setup[I810_CTXREG_MC0] = ( GFX_OP_MAP_COLOR_STAGES |
				     MC_STAGE_0 |
				     MC_UPDATE_DEST |
				     MC_DEST_CURRENT |
				     MC_UPDATE_ARG1 |
				     MC_ARG1_ITERATED_COLOR | 
				     MC_ARG1_DONT_REPLICATE_ALPHA |
				     MC_ARG1_DONT_INVERT |
				     MC_UPDATE_ARG2 |
				     MC_ARG2_ONE |
				     MC_ARG2_DONT_REPLICATE_ALPHA |
				     MC_ARG2_DONT_INVERT |
				     MC_UPDATE_OP |
				     MC_OP_ARG1 );
				     
   imesa->Setup[I810_CTXREG_MC1] = ( GFX_OP_MAP_COLOR_STAGES |
				     MC_STAGE_1 |
				     MC_UPDATE_DEST |
				     MC_DEST_CURRENT |
				     MC_UPDATE_ARG1 |
				     MC_ARG1_ONE | 
				     MC_ARG1_DONT_REPLICATE_ALPHA |
				     MC_ARG1_DONT_INVERT |
				     MC_UPDATE_ARG2 |
				     MC_ARG2_ONE |
				     MC_ARG2_DONT_REPLICATE_ALPHA |
				     MC_ARG2_DONT_INVERT |
				     MC_UPDATE_OP |
				     MC_OP_DISABLE );
				     

   imesa->Setup[I810_CTXREG_MC2] = ( GFX_OP_MAP_COLOR_STAGES |
				     MC_STAGE_2 |
				     MC_UPDATE_DEST |
				     MC_DEST_CURRENT |
				     MC_UPDATE_ARG1 |
				     MC_ARG1_CURRENT_COLOR | 
				     MC_ARG1_REPLICATE_ALPHA |
				     MC_ARG1_DONT_INVERT |
				     MC_UPDATE_ARG2 |
				     MC_ARG2_ONE |
				     MC_ARG2_DONT_REPLICATE_ALPHA |
				     MC_ARG2_DONT_INVERT |
				     MC_UPDATE_OP |
				     MC_OP_DISABLE );
				     

   imesa->Setup[I810_CTXREG_MA0] = ( GFX_OP_MAP_ALPHA_STAGES |
				     MA_STAGE_0 |
				     MA_UPDATE_ARG1 |
				     MA_ARG1_CURRENT_ALPHA |
				     MA_ARG1_DONT_INVERT |
				     MA_UPDATE_ARG2 |
				     MA_ARG2_CURRENT_ALPHA |
				     MA_ARG2_DONT_INVERT |
				     MA_UPDATE_OP |
				     MA_OP_ARG1 );


   imesa->Setup[I810_CTXREG_MA1] = ( GFX_OP_MAP_ALPHA_STAGES |
				     MA_STAGE_1 |
				     MA_UPDATE_ARG1 |
				     MA_ARG1_CURRENT_ALPHA |
				     MA_ARG1_DONT_INVERT |
				     MA_UPDATE_ARG2 |
				     MA_ARG2_CURRENT_ALPHA |
				     MA_ARG2_DONT_INVERT |
				     MA_UPDATE_OP |
				     MA_OP_ARG1 );


   imesa->Setup[I810_CTXREG_MA2] = ( GFX_OP_MAP_ALPHA_STAGES |
				     MA_STAGE_2 |
				     MA_UPDATE_ARG1 |
				     MA_ARG1_CURRENT_ALPHA |
				     MA_ARG1_DONT_INVERT |
				     MA_UPDATE_ARG2 |
				     MA_ARG2_CURRENT_ALPHA |
				     MA_ARG2_DONT_INVERT |
				     MA_UPDATE_OP |
				     MA_OP_ARG1 );


   imesa->Setup[I810_CTXREG_SDM] = ( GFX_OP_SRC_DEST_MONO |
				     SDM_UPDATE_MONO_ENABLE |
				     0 |
				     SDM_UPDATE_SRC_BLEND | 
				     SDM_SRC_ONE |
				     SDM_UPDATE_DST_BLEND |
				     SDM_DST_ZERO );

   /* Use for colormask:
    */
   imesa->Setup[I810_CTXREG_CF0] = GFX_OP_COLOR_FACTOR;
   imesa->Setup[I810_CTXREG_CF1] = 0xffffffff;

   imesa->Setup[I810_CTXREG_ZA] = (GFX_OP_ZBIAS_ALPHAFUNC |
				   ZA_UPDATE_ALPHAFUNC |
				   ZA_ALPHA_ALWAYS |
				   ZA_UPDATE_ZBIAS |
				   0 |
				   ZA_UPDATE_ALPHAREF |
				   0x0);

   imesa->Setup[I810_CTXREG_FOG] = (GFX_OP_FOG_COLOR | 
				    (0xffffff & ~FOG_RESERVED_MASK));

   /* Choose a pipe
    */
   imesa->Setup[I810_CTXREG_B1] = ( GFX_OP_BOOL_1 |
				    B1_UPDATE_SPEC_SETUP_ENABLE |
				    0 |
				    B1_UPDATE_ALPHA_SETUP_ENABLE |
				    B1_ALPHA_SETUP_ENABLE |
				    B1_UPDATE_CI_KEY_ENABLE |
				    0 |
				    B1_UPDATE_CHROMAKEY_ENABLE |
				    0 |
				    B1_UPDATE_Z_BIAS_ENABLE |
				    0 |
				    B1_UPDATE_SPEC_ENABLE |
				    0 |
				    B1_UPDATE_FOG_ENABLE |
				    0 |
				    B1_UPDATE_ALPHA_TEST_ENABLE |
				    0 |
				    B1_UPDATE_BLEND_ENABLE |
				    0 |
				    B1_UPDATE_Z_TEST_ENABLE |
				    0 );

   imesa->Setup[I810_CTXREG_B2] = ( GFX_OP_BOOL_2 |
				    B2_UPDATE_MAP_CACHE_ENABLE |
				    B2_MAP_CACHE_ENABLE |
				    B2_UPDATE_ALPHA_DITHER_ENABLE |
				    0 |
				    B2_UPDATE_FOG_DITHER_ENABLE |
				    0 |
				    B2_UPDATE_SPEC_DITHER_ENABLE |
				    0 |
				    B2_UPDATE_RGB_DITHER_ENABLE |
				    B2_RGB_DITHER_ENABLE |
				    B2_UPDATE_FB_WRITE_ENABLE |
				    B2_FB_WRITE_ENABLE |
				    B2_UPDATE_ZB_WRITE_ENABLE |
				    B2_ZB_WRITE_ENABLE );

   imesa->Setup[I810_CTXREG_LCS] = ( GFX_OP_LINEWIDTH_CULL_SHADE_MODE |
				     LCS_UPDATE_ZMODE |
				     LCS_Z_LESS |
				     LCS_UPDATE_LINEWIDTH |
				     (0x2<<LCS_LINEWIDTH_SHIFT) |
				     LCS_UPDATE_ALPHA_INTERP |
				     LCS_ALPHA_INTERP |
				     LCS_UPDATE_FOG_INTERP |
				     0 |
				     LCS_UPDATE_SPEC_INTERP |
				     0 |
				     LCS_UPDATE_RGB_INTERP |
				     LCS_RGB_INTERP |
				     LCS_UPDATE_CULL_MODE |
				     LCS_CULL_DISABLE);

   imesa->LcsCullMode = LCS_CULL_CW;
   
   imesa->Setup[I810_CTXREG_PV] = ( GFX_OP_PV_RULE |
				    PV_UPDATE_PIXRULE |
				    PV_PIXRULE_ENABLE |
				    PV_UPDATE_LINELIST |
				    PV_LINELIST_PV0 |
				    PV_UPDATE_TRIFAN |
				    PV_TRIFAN_PV0 |
				    PV_UPDATE_TRISTRIP |
				    PV_TRISTRIP_PV0 );


   imesa->Setup[I810_CTXREG_ST0] = GFX_OP_STIPPLE;
   imesa->Setup[I810_CTXREG_ST1] = 0;



   imesa->Setup[I810_CTXREG_AA] = ( GFX_OP_ANTIALIAS |
				    AA_UPDATE_EDGEFLAG |
				    0 |
				    AA_UPDATE_POLYWIDTH |
				    AA_POLYWIDTH_05 |
				    AA_UPDATE_LINEWIDTH |
				    AA_LINEWIDTH_05 |
				    AA_UPDATE_BB_EXPANSION |
				    0 |
				    AA_UPDATE_AA_ENABLE |
				    0 );



   memset(imesa->ClipSetup, 0, sizeof(imesa->ClipSetup));
   imesa->ClipSetup[I810_CLIPREG_SCI0] = GFX_OP_SCISSOR_INFO; 
   imesa->ClipSetup[I810_CLIPREG_SCI1] = 0;
   imesa->ClipSetup[I810_CLIPREG_SCI2] = 0;
   imesa->ClipSetup[I810_CLIPREG_SC] = ( GFX_OP_SCISSOR | 
					 SC_UPDATE_SCISSOR |
					 0 );

   /* This stuff is all invarient as long as we are using
    * shared back and depth buffers.  
    */
   memset(imesa->BufferSetup, 0, sizeof(imesa->BufferSetup));
   imesa->drawOffset = i810Screen->backOffset;
   imesa->BufferSetup[I810_DESTREG_DI0] = CMD_OP_DESTBUFFER_INFO;

   if (imesa->glCtx->Color.DriverDrawBuffer == GL_BACK_LEFT)
      imesa->BufferSetup[I810_DESTREG_DI1] = (i810Screen->backOffset | 
					      i810Screen->auxPitchBits);
   else
      imesa->BufferSetup[I810_DESTREG_DI1] = (i810Screen->fbOffset | 
					      i810Screen->auxPitchBits);


   imesa->BufferSetup[I810_DESTREG_DV0] = GFX_OP_DESTBUFFER_VARS;
   imesa->BufferSetup[I810_DESTREG_DV1] = (DV_HORG_BIAS_OGL |
					   DV_VORG_BIAS_OGL |
					   i810Screen->fbFormat);
   imesa->BufferSetup[I810_DESTREG_ZB0] = CMD_OP_Z_BUFFER_INFO;
   imesa->BufferSetup[I810_DESTREG_ZB1] = (i810Screen->depthOffset | 
					   i810Screen->auxPitchBits);

   imesa->BufferSetup[I810_DESTREG_DR0] = GFX_OP_DRAWRECT_INFO;
   imesa->BufferSetup[I810_DESTREG_DR1] = DR1_RECT_CLIP_ENABLE;
}


#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|\
                      NEW_TEXTURE_MATRIX|\
                      NEW_USER_CLIP|NEW_CLIENT_STATE|\
                      NEW_TEXTURE_ENABLE))

void i810DDUpdateState( GLcontext *ctx )
{
   i810ContextPtr imesa = I810_CONTEXT( ctx );

   if (ctx->NewState & INTERESTED) {
      i810DDChooseRenderState(ctx);  
      i810ChooseRasterSetupFunc(ctx);
   }
   
   /* TODO: stop mesa from clobbering these.
    */
   ctx->Driver.PointsFunc=imesa->PointsFunc;
   ctx->Driver.LineFunc=imesa->LineFunc;
   ctx->Driver.TriangleFunc=imesa->TriangleFunc;
   ctx->Driver.QuadFunc=imesa->QuadFunc;
}


void i810DDInitStateFuncs(GLcontext *ctx)
{
   ctx->Driver.UpdateState = i810DDUpdateState;
   ctx->Driver.Enable = i810DDEnable;
   ctx->Driver.LightModelfv = i810DDLightModelfv;
   ctx->Driver.AlphaFunc = i810DDAlphaFunc;
   ctx->Driver.BlendEquation = i810DDBlendEquation;
   ctx->Driver.BlendFunc = i810DDBlendFunc;
   ctx->Driver.BlendFuncSeparate = i810DDBlendFuncSeparate;
   ctx->Driver.DepthFunc = i810DDDepthFunc;
   ctx->Driver.DepthMask = i810DDDepthMask;
   ctx->Driver.Fogfv = i810DDFogfv;
   ctx->Driver.Scissor = i810DDScissor;
   ctx->Driver.CullFace = i810DDCullFaceFrontFace;
   ctx->Driver.FrontFace = i810DDCullFaceFrontFace;
   ctx->Driver.ColorMask = i810DDColorMask;
   ctx->Driver.ReducedPrimitiveChange = i810DDReducedPrimitiveChange;
   ctx->Driver.RenderStart = i810DDUpdateHwState; 
   ctx->Driver.RenderFinish = 0; 

   ctx->Driver.SetBuffer = i810DDSetBuffer;
   ctx->Driver.Color = i810DDSetColor;
   ctx->Driver.ClearColor = i810DDClearColor;
   ctx->Driver.Dither = i810DDDither;

   ctx->Driver.Index = 0;
   ctx->Driver.ClearIndex = 0;
   ctx->Driver.IndexMask = 0;

}

