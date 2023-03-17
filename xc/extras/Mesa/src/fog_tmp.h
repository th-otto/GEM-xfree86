
/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



static void TAG(fog_rgba_vertices)( struct vertex_buffer *VB,
				    GLuint side,
				    GLubyte flag)
{
   const GLcontext *ctx = VB->ctx;
   GLfloat rFog = ctx->Fog.Color[0];
   GLfloat gFog = ctx->Fog.Color[1];
   GLfloat bFog = ctx->Fog.Color[2];
   GLfloat end  = ctx->Fog.End;
   GLubyte *cullmask = VB->CullMask + VB->Start;
   GLfloat *v = VB->EyePtr->start;
   GLuint stride = VB->EyePtr->stride;
   GLuint n = VB->EyePtr->count;
   GLubyte *in;
   GLuint in_stride;
   GLubyte (*out)[4];
   GLfloat d,t;
   GLuint i;

   (void) cullmask;
   (void) flag;

   /* Get correct source and destination for fogged colors.
    */
   in_stride = VB->Color[side]->stride;
   in = VB->Color[side]->start;
   VB->Color[side] = VB->FoggedColor[side];
   VB->ColorPtr = VB->Color[0];
   out = (GLubyte (*)[4])VB->Color[side]->start;


   if (VB->EyePtr->size > 2) {
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
	 d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
	 for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride), in += in_stride) {
	    CULLCHECK {
	       GLfloat f = (end - ABSF(v[2])) * d;
	       if (f >= 1.0) continue;
	       if (f < 0) {
		  CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][0], rFog);
		  CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][1], gFog);
		  CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][2], bFog);
	       } else {
		  t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[0]) + (1.0F-f)*rFog;
		  CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][0], t);
	       
		  t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[1]) + (1.0F-f)*gFog;
		  CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][1], t);
	       
		  t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[2]) + (1.0F-f)*bFog;
		  CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][2], t);

	       }
	    }
	 }
	 break;
      case GL_EXP:
	 d = -ctx->Fog.Density;
	 for ( i = 0 ; i < n ; i++, STRIDE_F(v,stride), in += in_stride) {
	    CULLCHECK {
	       GLfloat f = exp( d*ABSF(v[2]) ); /* already clamped */ 

	       t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[0]) + (1.0F-f)*rFog;
	       CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][0], t);
	       
	       t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[1]) + (1.0F-f)*gFog;
	       CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][1], t);
	       
	       t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[2]) + (1.0F-f)*bFog;
	       CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][2], t);
	    }
	 }
	 break;
      case GL_EXP2:
	 d = -(ctx->Fog.Density*ctx->Fog.Density);
	 for ( i = 0 ; i < n ; i++, STRIDE_F(v, stride), in += in_stride) {
	    CULLCHECK {
	       GLfloat z = v[2];
	       GLfloat f = exp( d*z*z ); /* already clamped */

	       t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[0]) + (1.0F-f)*rFog;
	       CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][0], t);
	       
	       t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[1]) + (1.0F-f)*gFog;
	       CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][1], t);
	       
	       t = f * UBYTE_COLOR_TO_FLOAT_COLOR(in[2]) + (1.0F-f)*bFog;
	       CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(out[i][2], t);
	    }
	 }
	 break;
	 default:
	    gl_problem(ctx, "Bad fog mode in gl_fog_rgba_vertices");
	 return;
      }
   }
   else if (ctx->Fog.Mode == GL_LINEAR)
   {
      /* 2-vector vertices */
      GLubyte r,g,b;
      GLfloat f = ctx->Fog.End * (ctx->Fog.End - ctx->Fog.Start);
      CLAMP_FLOAT_COLOR( f );
      f = 1.0 - f;
      rFog *= f;
      bFog *= f;
      gFog *= f;

      CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(r, rFog);
      CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(g, gFog);
      CLAMPED_FLOAT_COLOR_TO_UBYTE_COLOR(b, bFog);

      for (i = 0 ; i < n ; i++) {
	 /* CULLCHECK */ {
	    out[i][0] = r;
	    out[i][1] = g;
	    out[i][2] = b;
	 }
      }
   }
}



/*
 * Compute the fogged color indexes for an array of vertices.
 * Input:  n - number of vertices
 *         v - array of vertices
 * In/Out: indx - array of vertex color indexes
 */
static void TAG(fog_ci_vertices)( struct vertex_buffer *VB,
				  GLuint side,
				  GLubyte flag )
{
   GLcontext *ctx = VB->ctx;

   GLubyte *cullmask = VB->CullMask + VB->Start;
  
   GLfloat *v = VB->EyePtr->start;
   GLuint stride = VB->EyePtr->stride;
   GLuint vertex_size = VB->EyePtr->size;
   GLuint n = VB->EyePtr->count;

   GLuint *in;
   GLuint in_stride;
   GLuint *out;
   GLuint i;

   (void) flag;
   (void) cullmask;


   in = VB->Index[side]->start;
   in_stride = VB->Index[side]->stride;
   VB->IndexPtr = VB->FoggedIndex[side];
   out = VB->IndexPtr->start;


   /* NOTE: the extensive use of casts generates better/faster code for MIPS */
   if (vertex_size > 2) {
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
      {
	 GLfloat d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
	 GLfloat fogindex = ctx->Fog.Index;
	 GLfloat fogend = ctx->Fog.End;

 	 for ( i = 0; i < n ; i++, STRIDE_F(v,stride), STRIDE_UI(in,in_stride))
	 {
	    CULLCHECK {
	       GLfloat f = (fogend - ABSF(v[2])) * d;
	       f = CLAMP( f, 0.0, 1.0 );
	       *out = (GLint) ((GLfloat)(GLint) *in + (1.0F-f) * fogindex);
	    }
	 }
	 break;
      }
      case GL_EXP:
      {
	 GLfloat d = -ctx->Fog.Density;
	 GLfloat fogindex = ctx->Fog.Index;
 	 for ( i = 0; i < n ; i++, STRIDE_F(v,stride), STRIDE_UI(in,in_stride))
	 {
	    CULLCHECK {
	       GLfloat f = exp( d * ABSF(v[2]) );
	       *out = (GLint) ((GLfloat)(GLint) *in + (1.0F-f) * fogindex);
	    }
	 }
	 break;
      }
      case GL_EXP2:
      {
	 GLfloat d = -(ctx->Fog.Density*ctx->Fog.Density);
	 GLfloat fogindex = ctx->Fog.Index;
 	 for ( i = 0; i < n ; i++, STRIDE_F(v,stride),STRIDE_UI(in,in_stride))
	 {
	    CULLCHECK {
	       GLfloat z = v[2]; /*ABSF(v[i][2]);*/
	       GLfloat f = exp( d * z*z ); /* was -d ??? */
	       out[i] = (GLint) ((GLfloat)(GLint) *in + (1.0F-f) * fogindex);
	    }
	 }
	 break;
      }
      default:
	 gl_problem(ctx, "Bad fog mode in gl_fog_ci_vertices");
	 return;
      }
   } else if (ctx->Fog.Mode == GL_LINEAR) {
      GLuint fogindex;
      GLfloat f = ctx->Fog.End / (ctx->Fog.End - ctx->Fog.Start);

      f = 1.0 - CLAMP( f, 0.0F, 1.0F );
      fogindex = (GLuint)(f * ctx->Fog.Index);
      if (fogindex) {
 	 for ( i = 0; i < n ; i++, STRIDE_F(v,stride), STRIDE_UI(in,in_stride))
	 {
	    CULLCHECK {
	       *out = *in + fogindex;
	    }
	 }
      }
   } 
}

static void TAG(init_fog_tab)(void)
{
   fog_ci_tab[IDX] = TAG(fog_ci_vertices);
   fog_rgba_tab[IDX] = TAG(fog_rgba_vertices);
}

#undef TAG
#undef CULLCHECK
#undef IDX
