/* -*- mode: C; tab-width:8; c-basic-offset:2 -*- */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */

 
#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "glide.h"
#include "fxglidew.h"
#include "fxdrv.h"

#include <stdlib.h>
#include <string.h>

FxI32 FX_grGetInteger_NoLock(FxU32 pname)
{
#if !defined(FX_GLIDE3)
  switch (pname) 
  {
    case FX_FOG_TABLE_ENTRIES:
       return GR_FOG_TABLE_SIZE;
    case FX_GLIDE_STATE_SIZE:
       return sizeof(GrState);
    case FX_LFB_PIXEL_PIPE:
       return FXFALSE;
    case FX_PENDING_BUFFERSWAPS:
	return grBufferNumPending();
    case FX_TEXTURE_ALIGN:
        /* This is a guess from reading the glide3 docs */
        return 8;
    default:
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
          fprintf(stderr,"Wrong parameter in FX_grGetInteger!\n");
       }
       return -1;
  }
#else
  FxU32 grname;
  FxI32 result;
  
  switch (pname)
  {
     case FX_FOG_TABLE_ENTRIES:
     case FX_GLIDE_STATE_SIZE:
     case FX_LFB_PIXEL_PIPE:
     case FX_PENDING_BUFFERSWAPS:
     case FX_TEXTURE_ALIGN:
       grname = pname;
       break;
     default:
       if (MESA_VERBOSE&VERBOSE_DRIVER) {
          fprintf(stderr,"Wrong parameter in FX_grGetInteger!\n");
       }
       return -1;
  }

  grGet(grname,4,&result);
  return result;
#endif
}

FxI32 FX_grGetInteger(FxU32 pname)
{
  int result;

  BEGIN_BOARD_LOCK();
  result=FX_grGetInteger_NoLock(pname);
  END_BOARD_LOCK();
  return result;
}


FxBool FX_grLfbLock(GrLock_t type, GrBuffer_t buffer, 
		    GrLfbWriteMode_t writeMode, GrOriginLocation_t origin, 
		    FxBool pixelPipeline, GrLfbInfo_t *info ) {
  FxBool result;

  BEGIN_BOARD_LOCK();
  result=grLfbLock(type, buffer, writeMode, origin, pixelPipeline, info);
  END_BOARD_LOCK();
  return result;
}

FxU32 FX_grTexTextureMemRequired(FxU32 evenOdd, GrTexInfo *info) {
  FxU32 result;

  BEGIN_BOARD_LOCK();
  result=grTexTextureMemRequired(evenOdd, info);
  END_BOARD_LOCK();
  return result;
}

FxU32 FX_grTexMinAddress(GrChipID_t tmu) {
  FxU32 result;

  BEGIN_BOARD_LOCK();
  result=grTexMinAddress(tmu);
  END_BOARD_LOCK();
  return result;
}

extern FxU32 FX_grTexMaxAddress(GrChipID_t tmu) {
  FxU32 result;

  BEGIN_BOARD_LOCK();
  result=grTexMaxAddress(tmu);
  END_BOARD_LOCK();
  return result;
}

FxBool FX_grSstControl(FxU32 code)
{
#if defined(FX_GLIDE3)
  (void) code;
  return 1;  /* OK? */
#else
  FxU32 result;
  BEGIN_BOARD_LOCK();
  result = grSstControl(code);
  END_BOARD_LOCK();
  return result;
#endif
}


#if defined(FX_GLIDE3)

void FX_grGammaCorrectionValue(float val)
{
  (void)val;
/* ToDo */
}

int FX_getFogTableSize(void)
{
   int result;
   BEGIN_BOARD_LOCK();
   grGet(GR_FOG_TABLE_ENTRIES,sizeof(int),(void*)&result);
   END_BOARD_LOCK();
   return result; 
}

int FX_getGrStateSize(void)
{
   int result;
   BEGIN_BOARD_LOCK();
   grGet(GR_GLIDE_STATE_SIZE,sizeof(int),(void*)&result);
   END_BOARD_LOCK();

   return result;
   
}

int FX_grSstScreenWidth()
{
   FxI32 result[4];

   BEGIN_BOARD_LOCK();
   grGet(GR_VIEWPORT,sizeof(FxI32)*4,result);
   END_BOARD_LOCK();
   
   return result[2];
}

int FX_grSstScreenHeight()
{
   FxI32 result[4];

   BEGIN_BOARD_LOCK();
   grGet(GR_VIEWPORT,sizeof(FxI32)*4,result);
   END_BOARD_LOCK();
   
   return result[3];
}

void FX_grGlideGetVersion(char *buf)
{
  BEGIN_BOARD_LOCK();
  strcpy(buf,grGetString(GR_VERSION));
  END_BOARD_LOCK();
}

void FX_grSstPerfStats(GrSstPerfStats_t *st)
{
  /* ToDo */
  st->pixelsIn = 0;
  st->chromaFail = 0;
  st->zFuncFail = 0;
  st->aFuncFail = 0;
  st->pixelsOut = 0;
}

void FX_grAADrawLine(GrVertex *a,GrVertex *b)
{
   /* ToDo */
   BEGIN_CLIP_LOOP();
   grDrawLine(a,b);
   END_CLIP_LOOP();
}

void FX_grAADrawPoint(GrVertex *a)
{
  BEGIN_CLIP_LOOP();
  grDrawPoint(a);
  END_CLIP_LOOP();
}

void FX_grDrawPolygonVertexList(int n, GrVertex *verts) 
{
  BEGIN_CLIP_LOOP();
  grDrawVertexArrayContiguous(GR_POLYGON, n, verts, sizeof(GrVertex));
  END_CLIP_LOOP();
}

#if FX_USE_PARGB
void FX_setupGrVertexLayout(void)
{
  BEGIN_BOARD_LOCK();
  grReset(GR_VERTEX_PARAMETER);
   
  grCoordinateSpace(GR_WINDOW_COORDS);
  grVertexLayout(GR_PARAM_XY, GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_PARGB, GR_VERTEX_PARGB_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q, GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Z, GR_VERTEX_OOZ_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);	
  grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2, GR_PARAM_DISABLE); 
  grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);	
  grVertexLayout(GR_PARAM_Q1, GR_VERTEX_OOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);	
  END_BOARD_LOCK();
}
#else /* FX_USE_PARGB */
void FX_setupGrVertexLayout(void)
{
  BEGIN_BOARD_LOCK();
  grReset(GR_VERTEX_PARAMETER);
   
  grCoordinateSpace(GR_WINDOW_COORDS);
  grVertexLayout(GR_PARAM_XY, GR_VERTEX_X_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_RGB, GR_VERTEX_R_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_A, GR_VERTEX_A_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Q, GR_VERTEX_OOW_OFFSET << 2, GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_Z, GR_VERTEX_OOZ_OFFSET << 2,	GR_PARAM_ENABLE);
  grVertexLayout(GR_PARAM_ST0, GR_VERTEX_SOW_TMU0_OFFSET << 2, GR_PARAM_ENABLE);	
  grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2, GR_PARAM_DISABLE);
  grVertexLayout(GR_PARAM_ST1, GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);	
  grVertexLayout(GR_PARAM_Q1, GR_VERTEX_OOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);
  END_BOARD_LOCK();
}
#endif

void FX_grHints_NoLock(GrHint_t hintType, FxU32 hintMask)
{
  switch(hintType) {
  case GR_HINT_STWHINT:
    {
      if (hintMask & GR_STWHINT_W_DIFF_TMU0)
	grVertexLayout(GR_PARAM_Q0, GR_VERTEX_OOW_TMU0_OFFSET << 2, 	GR_PARAM_ENABLE);
      else
	grVertexLayout(GR_PARAM_Q0,GR_VERTEX_OOW_TMU0_OFFSET << 2, 	GR_PARAM_DISABLE);
      
      if (hintMask & GR_STWHINT_ST_DIFF_TMU1)
	grVertexLayout(GR_PARAM_ST1,GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_ENABLE);
      else
	grVertexLayout(GR_PARAM_ST1,GR_VERTEX_SOW_TMU1_OFFSET << 2, GR_PARAM_DISABLE);
        
      if (hintMask & GR_STWHINT_W_DIFF_TMU1)
	grVertexLayout(GR_PARAM_Q1,GR_VERTEX_OOW_TMU1_OFFSET << 2,	GR_PARAM_ENABLE);
      else
	grVertexLayout(GR_PARAM_Q1,GR_VERTEX_OOW_TMU1_OFFSET << 2,	GR_PARAM_DISABLE);
      
    }
  }
}

void FX_grHints(GrHint_t hintType, FxU32 hintMask) {
  BEGIN_BOARD_LOCK();
  FX_grHints_NoLock(hintType, hintMask);
  END_BOARD_LOCK();
}

int FX_grSstQueryHardware(GrHwConfiguration *config)
{
   int i,j;
   int numFB;

   BEGIN_BOARD_LOCK();
   grGet(GR_NUM_BOARDS,4,(void*)&(config->num_sst));
   if (config->num_sst == 0)
   	return 0;
   for (i = 0; i< config->num_sst; i++)
   {
      config->SSTs[i].type = GR_SSTTYPE_VOODOO;
      grSstSelect(i);
      grGet(GR_MEMORY_FB,4,(void*)&(config->SSTs[i].sstBoard.VoodooConfig.fbRam));
      config->SSTs[i].sstBoard.VoodooConfig.fbRam/= 1024*1024;
      
      grGet(GR_NUM_TMU,4,(void*)&(config->SSTs[i].sstBoard.VoodooConfig.nTexelfx));
   
      
      grGet(GR_NUM_FB,4,(void*)&numFB);
      if (numFB > 1)
         config->SSTs[i].sstBoard.VoodooConfig.sliDetect = FXTRUE;
      else
         config->SSTs[i].sstBoard.VoodooConfig.sliDetect = FXFALSE;
      for (j = 0; j < config->SSTs[i].sstBoard.VoodooConfig.nTexelfx; j++)
      {
      	 grGet(GR_MEMORY_TMU,4,(void*)&(config->SSTs[i].sstBoard.VoodooConfig.tmuConfig[j].tmuRam));
      	 config->SSTs[i].sstBoard.VoodooConfig.tmuConfig[j].tmuRam /= 1024*1024;
      }
   }
   END_BOARD_LOCK();
   return 1;
}

#else

int FX_grSstScreenWidth()
{
   int i;
   BEGIN_BOARD_LOCK();
   i = grSstScreenWidth();
   END_BOARD_LOCK();
   return i;
}

int FX_grSstScreenHeight()
{
   int i;
   BEGIN_BOARD_LOCK();
   i = grSstScreenHeight();
   END_BOARD_LOCK();
   return i;
}

int FX_grSstQueryHardware(GrHwConfiguration *c)	
{
   int i;
   BEGIN_BOARD_LOCK();
   i = grSstQueryHardware(c);
   END_BOARD_LOCK();
   return i;
} 

FX_GrContext_t FX_grSstWinOpen( FxU32                hWnd,
                                GrScreenResolution_t screen_resolution,
                                GrScreenRefresh_t    refresh_rate,
                                GrColorFormat_t      color_format,
                                GrOriginLocation_t   origin_location,
                                int                  nColBuffers,
                                int                  nAuxBuffers)
{
   FX_GrContext_t i;
   BEGIN_BOARD_LOCK();
   i = grSstWinOpen( hWnd,
                     screen_resolution,
                     refresh_rate,
                     color_format,
                     origin_location,
                     nColBuffers,
                     nAuxBuffers );
   
   fprintf(stderr, 
           "grSstWinOpen( win %d res %d ref %d fmt %d\n"
           "              org %d ncol %d naux %d )\n"
           " ==> %d\n",
           hWnd,
           screen_resolution,
           refresh_rate,
           color_format,
           origin_location,
           nColBuffers,
           nAuxBuffers,
           i);
   END_BOARD_LOCK();
   return i;
}



#endif 
#else

/*
 * Need this to provide at least one external definition.
 */

int gl_fx_dummy_function_glidew(void)
{
  return 0;
}

#endif /* FX */
