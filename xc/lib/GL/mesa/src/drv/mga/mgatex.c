/* -*- mode: C; c-basic-offset:8 -*- */
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
 *    original by Wittawat Yamwong <Wittawat.Yamwong@stud.uni-hannover.de>
 *	9/20/99 rewrite by John Carmack <johnc@idsoftware.com>
 *      13/1/00 port to DRI by Keith Whitwell <keithw@precisioninsight.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <GL/gl.h>

#include "mm.h"
#include "mgalib.h"
#include "mgadma.h"
#include "mgatex.h"
#include "mgalog.h"
#include "mgaregs.h"

#include "simple_list.h"


/*
 * mgaDestroyTexObj
 * Free all memory associated with a texture and NULL any pointers
 * to it.
 */
static void mgaDestroyTexObj( mgaContextPtr mmesa, mgaTextureObjectPtr t ) {
	int	i;

 	if ( !t ) return;
  	  	
	/* free the texture memory */
	mmFreeMem( t->MemBlock );
 
 	/* free mesa's link */   
	t->tObj->DriverData = NULL;

	/* see if it was the driver's current object */
	for ( i = 0 ; i < 2 ; i++ ) {
		if ( mmesa->CurrentTexObj[i] == t ) {
			mmesa->CurrentTexObj[i] = NULL;
		}  
	}
	
	remove_from_list(t);
	free( t );
}

static void mgaSwapOutTexObj(mgaContextPtr mmesa, mgaTextureObjectPtr t)
{
	if (t->MemBlock) {
		mmFreeMem(t->MemBlock);
		t->MemBlock = 0;      
	}

	t->dirty_images = ~0;
	move_to_tail(&(mmesa->SwappedOut), t);
}


void mgaResetGlobalLRU( mgaContextPtr mmesa )
{
   mgaTexRegion *list = mmesa->sarea->texList;
   int sz = 1 << mmesa->mgaScreen->logTextureGranularity;
   int i;

   /* (Re)initialize the global circular LRU list.  The last element
    * in the array (MGA_NR_TEX_REGIONS) is the sentinal.  Keeping it
    * at the end of the array allows it to be addressed rationally
    * when looking up objects at a particular location in texture
    * memory.  
    */
   for (i = 0 ; (i+1) * sz < mmesa->mgaScreen->textureSize ; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
   }

   i--;
   list[0].prev = MGA_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = MGA_NR_TEX_REGIONS;
   list[MGA_NR_TEX_REGIONS].prev = i;
   list[MGA_NR_TEX_REGIONS].next = 0;
   mmesa->sarea->texAge = 0;
}


static void mgaUpdateTexLRU( mgaContextPtr mmesa, mgaTextureObjectPtr t ) 
{
   int i;
   int logsz = mmesa->mgaScreen->logTextureGranularity;
   int start = t->MemBlock->ofs >> logsz;
   int end = (t->MemBlock->ofs + t->MemBlock->size - 1) >> logsz;
   mgaTexRegion *list = mmesa->sarea->texList;
   
   mmesa->texAge = ++mmesa->sarea->texAge;

   /* Update our local LRU
    */
   move_to_head( &(mmesa->TexObjList), t );

   /* Update the global LRU
    */
   for (i = start ; i <= end ; i++) {

      list[i].in_use = 1;
      list[i].age = mmesa->texAge;

      /* remove_from_list(i)
       */
      list[(unsigned)list[i].next].prev = list[i].prev;
      list[(unsigned)list[i].prev].next = list[i].next;
      
      /* insert_at_head(list, i)
       */
      list[i].prev = MGA_NR_TEX_REGIONS;
      list[i].next = list[MGA_NR_TEX_REGIONS].next;
      list[(unsigned)list[MGA_NR_TEX_REGIONS].next].prev = i;
      list[MGA_NR_TEX_REGIONS].next = i;
   }
}


/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent 
 * the other client's textures.  
 */
void mgaTexturesGone( mgaContextPtr mmesa,
		       GLuint offset, 
		       GLuint size,
		       GLuint in_use ) 
{
   mgaTextureObjectPtr t, tmp;
   
   foreach_s ( t, tmp, &mmesa->TexObjList ) {

      if (t->MemBlock->ofs >= offset + size ||
	  t->MemBlock->ofs + t->MemBlock->size <= offset)
	 continue;

      /* It overlaps - kick it off.  Need to hold onto the currently bound
       * objects, however.
       */
      if (t == mmesa->CurrentTexObj[0] || t == mmesa->CurrentTexObj[1]) 
	 mgaSwapOutTexObj( mmesa, t );
      else
	 mgaDestroyTexObj( mmesa, t );
   }

   
   if (in_use) {
      t = (mgaTextureObjectPtr) calloc(1,sizeof(*t));
      if (!t) return;

      t->MemBlock = mmAllocMem( mmesa->texHeap, size, 0, offset);      
      insert_at_head( &mmesa->TexObjList, t );
   }
}


/*
 * mgaSetTexWrappings
 */
static void mgaSetTexWrapping( mgaTextureObjectPtr t,
			       GLenum sWrap, GLenum tWrap ) {
	if (sWrap == GL_REPEAT) {
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_clampu_enable;
	} else {
		t->Setup[MGA_TEXREG_CTL] |= TMC_clampu_enable;
	}
	if (tWrap == GL_REPEAT) {
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_clampv_enable;
	} else {
		t->Setup[MGA_TEXREG_CTL] |= TMC_clampv_enable;
	}
}

/*
 * mgaSetTexFilter
 */
static void mgaSetTexFilter(mgaTextureObjectPtr t, GLenum minf, GLenum magf) {
	switch (minf) {
	case GL_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_nrst);
		break;
	case GL_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_bilin);
		break;
	case GL_NEAREST_MIPMAP_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm1s);
		break;
	case GL_LINEAR_MIPMAP_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm4s);
		break;
	case GL_NEAREST_MIPMAP_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm2s);
		break;
	case GL_LINEAR_MIPMAP_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_minfilter_MASK, TF_minfilter_mm8s);
		break;
	default:
		mgaError("mgaSetTexFilter(): not supported min. filter %d\n",
			 (int)minf);
		break;
	}

	switch (magf) {
	case GL_NEAREST:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_magfilter_MASK,TF_magfilter_nrst);
		break;
	case GL_LINEAR:
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],
		  TF_magfilter_MASK,TF_magfilter_bilin);
		break;
	default:
		mgaError("mgaSetTexFilter(): not supported mag. filter %d\n",
			 (int)magf);
		break;
	}
  
	/* See OpenGL 1.2 specification */
	if (magf == GL_LINEAR && (minf == GL_NEAREST_MIPMAP_NEAREST || 
                            minf == GL_NEAREST_MIPMAP_LINEAR)) {
		/* c = 0.5 */
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],TF_fthres_MASK,
		  0x20 << TF_fthres_SHIFT);
	} else {
		/* c = 0 */
		MGA_SET_FIELD(t->Setup[MGA_TEXREG_FILTER],TF_fthres_MASK,
		  0x10 << TF_fthres_SHIFT);
	}

}

/*
 * mgaSetTexBorderColor
 */
static void mgaSetTexBorderColor(mgaTextureObjectPtr t, GLubyte color[4]) {
	t->Setup[MGA_TEXREG_BORDERCOL] = 
		MGAPACKCOLOR8888(color[0],color[1],color[2],color[3]);

}







/*
 * mgaUploadSubImage
 *
 * Perform an iload based update of a resident buffer.  This is used for
 * both initial loading of the entire image, and texSubImage updates.
 *
 * Performed with the hardware lock held.
 */
static void mgaUploadSubImage( mgaContextPtr mmesa,
			       mgaTextureObjectPtr t,
			       int level,	     
			       int x, int y, int width, int height ) {
	int		x2;
	int		dwords;
        int		dstorg;
	struct gl_texture_image *image;
	int		texelBytes, texelsPerDword, texelMaccess;

	if ( level < 0 || level >= MGA_TEX_MAXLEVELS ) {
		mgaMsg( 1, "mgaUploadSubImage: bad level: %i\n", level );
 	 	return;
	}

	image = t->tObj->Image[level];
	if ( !image ) {
		mgaError( "mgaUploadSubImage: NULL image\n" );
		return;
	}
	
	/* find the proper destination offset for this level */
   	dstorg = (mmesa->mgaScreen->textureOffset + t->MemBlock->ofs + 
		  t->offsets[level]);

    	/* turn on PCI/AGP if needed 
    	if ( textureHeapPhysical ) {
    		dstorg |= 1 | mgaglx.use_agp;
    	}
	*/

	texelBytes = t->texelBytes;	
	switch( texelBytes ) {
	case 1:
		texelsPerDword = 4;
		texelMaccess = 0;
		break;
	case 2:
		texelsPerDword = 2;
		texelMaccess = 1;
		break;
	case 4:
		texelsPerDword = 1;
		texelMaccess = 2;
		break;
	default:
		return;
	}
			
		
	/* We can't do a subimage update if pitch is < 32 texels due
	 * to hardware XY addressing limits, so we will need to
	 * linearly upload all modified rows.
	 */
	if ( image->Width < 32 ) {
		x = 0;
		width = image->Width * height;
		height = 1;

		/* Assume that 1x1 textures aren't going to cause a
		 * bus error if we read up to four texels from that
		 * location: 
		 */
		if ( width < texelsPerDword ) {
			width = texelsPerDword;
		}
	} else {
		/* pad the size out to dwords.  The image is a pointer
		   to the entire image, so we can safely reference
		   outside the x,y,width,height bounds if we need to */
		x2 = x + width;
		x2 = (x2 + (texelsPerDword-1)) & ~(texelsPerDword-1);	
		x = (x + (texelsPerDword-1)) & ~(texelsPerDword-1);
		width = x2 - x;
	}
    
	/* we may not be able to upload the entire texture in one
	   batch due to register limits or dma buffer limits.
	   Recursively split it up. */
	while ( 1 ) {
		dwords = height * width / texelsPerDword;
		if ( dwords * 4 <= MGA_DMA_BUF_SZ ) {
			break;
		}
		mgaMsg(10, "mgaUploadSubImage: recursively subdividing\n" );

		mgaUploadSubImage( mmesa, t, level, x, y, width, height >> 1 );
		y += ( height >> 1 );
		height -= ( height >> 1 );
	}
	
	mgaMsg(10, "mgaUploadSubImage: %i,%i of %i,%i at %i,%i\n", 
	       width, height, image->Width, image->Height, x, y );

	/* bump the performance counter */
	mgaglx.c_textureSwaps += ( dwords << 2 );
	
	
#if 0

	/* fill in the secondary buffer with properly converted texels
	from the mesa buffer */
	mgaConvertTexture( dest, texelBytes, image, x, y, width, height );
		
	/* send the secondary data */	
	mgaSecondaryDma( TT_BLIT, dest, dwords );
#endif

}



static void mgaUploadTexLevel( mgaContextPtr mmesa, 
			       mgaTextureObjectPtr t,
			       int l )
{
	mgaUploadSubImage( mmesa,
			   t,
			   l,
			   0, 0,
			   t->tObj->Image[l]->Width,
			   t->tObj->Image[l]->Height);
}



/*
 * mgaCreateTexObj
 * Allocate space for and load the mesa images into the texture memory block.
 * This will happen before drawing with a new texture, or drawing with a
 * texture after it was swapped out or teximaged again.
 */
static void mgaCreateTexObj(mgaContextPtr mmesa, struct gl_texture_object *tObj) 
{
	mgaTextureObjectPtr		t;
	int					i, ofs, size;
	struct gl_texture_image *image;
	int					LastLevel;
	int					s, s2;
	int					textureFormat;
	
	mgaMsg( 10,"mgaCreateTexObj( %p )\n", tObj );

	t = malloc( sizeof( *t ) );
	if ( !t ) {
		mgaError( "mgaCreateTexObj: Failed to malloc mgaTextureObject\n" );
		return;
	}
	memset( t, 0, sizeof( *t ) );
	
	image = tObj->Image[ 0 ];
	if ( !image ) {
		return;
	}

	if ( 0 ) {
		/* G400 texture format options */

	} else {
		/* G200 texture format options */
		
		switch( image->Format ) {
		case GL_RGB:
	 	case GL_LUMINANCE:
	 		if ( image->IntFormat != GL_RGB5 &&
			     ( image->IntFormat == GL_RGB8 ||
			       mgaglx.default32BitTextures ) ) {
	 			t->texelBytes = 4;
	 			textureFormat = TMC_tformat_tw32;
	 		} else {
				t->texelBytes = 2;
				textureFormat = TMC_tformat_tw16;
			}
			break;
		case GL_ALPHA:
	 	case GL_LUMINANCE_ALPHA:
		case GL_INTENSITY:
		case GL_RGBA:
	 		if ( image->IntFormat != GL_RGBA4 &&
			     ( image->IntFormat == GL_RGBA8 ||
			       mgaglx.default32BitTextures ) ) {
	 			t->texelBytes = 4;
	 			textureFormat = TMC_tformat_tw32;
	 		} else {
				t->texelBytes = 2;
				textureFormat = TMC_tformat_tw12;
			}
			break;
		case GL_COLOR_INDEX:
			textureFormat = TMC_tformat_tw8;
			t->texelBytes = 1;
			break;
		default:
			mgaError( "mgaCreateTexObj: bad image->Format\n" );
			free( t );
			return;	
		}
	}
		
	/* we are going to upload all levels that are present, even if
	   later levels wouldn't be used by the current filtering mode.  This
	   allows the filtering mode to change without forcing another upload
	   of the images */
	LastLevel = MGA_TEX_MAXLEVELS-1;

	ofs = 0;
	for ( i = 0 ; i <= LastLevel ; i++ ) {
		int levelWidth, levelHeight;
		
		t->offsets[i] = ofs;
		image = tObj->Image[ i ];
		if ( !image ) {
			LastLevel = i - 1;
			mgaMsg( 10, "  missing images after LastLevel: %i\n",
				LastLevel );
			break;
		}
		/* the G400 doesn't work with textures less than 8
                   units in size */
		levelWidth = image->Width;
		levelHeight = image->Height;
		if ( levelWidth < 8 ) {
			levelWidth = 8;
		}
		if ( levelHeight < 8 ) {
			levelHeight = 8;
		}
		size = levelWidth * levelHeight * t->texelBytes;
		size = ( size + 31 ) & ~31;	/* 32 byte aligned */
		ofs += size;
		t->dirty_images |= (1<<i);
	}
	t->totalSize = ofs;
	t->lastLevel = LastLevel;
	

	/* fill in our mga texture object */
	t->tObj = tObj;
	t->ctx = mmesa;

	
	insert_at_tail(&(mmesa->TexObjList), t);
	
	t->MemBlock = 0;

	/* base image */	  
	image = tObj->Image[ 0 ];

	/* setup hardware register values */		
	t->Setup[MGA_TEXREG_CTL] = (TMC_takey_1 | 
				       TMC_tamask_0 | 
				       textureFormat );

	if (image->WidthLog2 >= 3) {
		t->Setup[MGA_TEXREG_CTL] |= ((image->WidthLog2 - 3) << 
						TMC_tpitch_SHIFT);
	} else {
		t->Setup[MGA_TEXREG_CTL] |= (TMC_tpitchlin_enable | 
						(image->Width << 
						 TMC_tpitchext_SHIFT));
	}


	t->Setup[MGA_TEXREG_CTL2] = TMC_ckstransdis_enable;

	if ( mmesa->glCtx->Light.Model.ColorControl == 
	     GL_SEPARATE_SPECULAR_COLOR ) 
	{
		t->Setup[MGA_TEXREG_CTL2] |= TMC_specen_enable;
	}
	
	t->Setup[MGA_TEXREG_FILTER] = (TF_minfilter_nrst | 
					  TF_magfilter_nrst |
					  TF_filteralpha_enable | 
					  (0x10 << TF_fthres_SHIFT) | 
					  (LastLevel << TF_mapnb_SHIFT));
	
	/* warp texture registers */
	if (MGA_IS_G200(mmesa)) {
		ofs = 28;
	} else {
   		ofs = 11;
   	}

	s = image->Width;
	s2 = image->WidthLog2;
	t->Setup[MGA_TEXREG_WIDTH] = 
		MGA_FIELD(TW_twmask, s - 1) |
		MGA_FIELD(TW_rfw,    (10 - s2 - 8) & 63 ) |
		MGA_FIELD(TW_tw,     (s2 + ofs ) | 0x40 );

	
	s = image->Height;
	s2 = image->HeightLog2;
	t->Setup[MGA_TEXREG_HEIGHT] = 
		MGA_FIELD(TH_thmask, s - 1) |
		MGA_FIELD(TH_rfh,    (10 - s2 - 8) & 63 ) | 
		MGA_FIELD(TH_th,     (s2 + ofs ) | 0x40 );


	/* set all the register values for filtering, border, etc */	
	mgaSetTexWrapping( t, tObj->WrapS, tObj->WrapT );
	mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
	mgaSetTexBorderColor( t, tObj->BorderColor );

  	tObj->DriverData = t;
}



int mgaUploadTexImages( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{

	int i;
	int ofs;
	mgaglx.c_textureSwaps++;

	/* Do we need to eject LRU texture objects?
	 */
	if (!t->MemBlock) {
		while (1)
		{
			t->MemBlock = mmAllocMem( mmesa->texHeap, t->totalSize, 12, 0 ); 
			if (t->MemBlock)
				break;

			if (mmesa->TexObjList.prev == &(mmesa->TexObjList)) {
				fprintf(stderr, "Failed to upload texture, sz %d\n", t->totalSize);
				mmDumpMemInfo( mmesa->texHeap );
				return -1;
			}

			mgaDestroyTexObj( mmesa, mmesa->TexObjList.prev );
		}
 
		ofs = t->MemBlock->ofs;

		t->Setup[MGA_TEXREG_ORG]  = ofs;
		t->Setup[MGA_TEXREG_ORG1] = ofs + t->offsets[1];
		t->Setup[MGA_TEXREG_ORG2] = ofs + t->offsets[2];
		t->Setup[MGA_TEXREG_ORG3] = ofs + t->offsets[3];
		t->Setup[MGA_TEXREG_ORG4] = ofs + t->offsets[4];

		mmesa->dirty |= MGA_UPLOAD_CTX;
	}

	/* Let the world know we've used this memory recently.
	 */
	mgaUpdateTexLRU( mmesa, t );

	if (t->dirty_images) {
		if (MGA_DEBUG & MGA_DEBUG_VERBOSE_MSG)
			fprintf(stderr, "*");

		for (i = 0 ; i <= t->lastLevel ; i++)
			if (t->dirty_images & (1<<i)) 
				mgaUploadTexLevel( mmesa, t, i );
	}


	t->dirty_images = 0;
	return 0;
}

/*
============================================================================

PUBLIC MGA FUNCTIONS

============================================================================
*/



static void mgaUpdateTextureEnvG200( GLcontext *ctx )
{
	struct gl_texture_object *tObj = ctx->Texture.Unit[0].Current;
	mgaTextureObjectPtr t;

	if (!tObj || !tObj->DriverData)
		return;

	t = (mgaTextureObjectPtr)tObj->DriverData;

	switch (ctx->Texture.Unit[0].EnvMode) {
	case GL_REPLACE:
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_tmodulate_enable;
		t->Setup[MGA_TEXREG_CTL2] &= ~TMC_decaldis_enable;
		break;
	case GL_MODULATE:
		t->Setup[MGA_TEXREG_CTL] |= TMC_tmodulate_enable;
		t->Setup[MGA_TEXREG_CTL2] &= ~TMC_decaldis_enable;
		break;
	case GL_DECAL:
		t->Setup[MGA_TEXREG_CTL] &= ~TMC_tmodulate_enable;
		t->Setup[MGA_TEXREG_CTL2] &= ~TMC_decaldis_enable;
		break;
	case GL_BLEND:
		t->ctx->Fallback |= MGA_FALLBACK_TEXTURE;
		break;
	default:
	}
}

/* I don't have the alpha values correct yet:
 */
static void mgaUpdateTextureStage( GLcontext *ctx, int unit )
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mgaUI32 *reg = &mmesa->Setup[MGA_CTXREG_TDUAL0 + unit];
	GLuint source = mmesa->tmu_source[unit];
	struct gl_texture_object *tObj = ctx->Texture.Unit[source].Current;

	*reg = 0;
	if (unit == 1)
		*reg = mmesa->Setup[MGA_CTXREG_TDUAL0];
	
	if ( tObj != ctx->Texture.Unit[source].CurrentD[2] ) 
		return;
	
	if ( ((ctx->Enabled>>(source*4))&TEXTURE0_ANY) != TEXTURE0_2D ) 
		return;
	
	if (!tObj || !tObj->Complete) 
		return;

	switch (ctx->Texture.Unit[source].EnvMode) {
	case GL_REPLACE:
		*reg = (TD0_color_sel_arg1 |
			TD0_alpha_sel_arg1 );
		break;

	case GL_MODULATE:
		if (unit == 0)
			*reg = ( TD0_color_arg2_diffuse |
				 TD0_color_sel_mul | 
				 TD0_alpha_arg2_diffuse |
				 TD0_alpha_sel_arg1);
		else
			*reg = ( TD0_color_arg2_prevstage |
				 TD0_color_alpha_prevstage |
				 TD0_color_sel_mul | 
				 TD0_alpha_arg2_prevstage |
				 TD0_alpha_sel_arg1);
		break;
	case GL_DECAL:
		*reg = (TD0_color_arg2_fcol | 
			TD0_color_alpha_currtex |
			TD0_color_alpha2inv_enable |
			TD0_color_arg2mul_alpha2 |
			TD0_color_arg1mul_alpha1 |
			TD0_color_add_add |
			TD0_color_sel_add |
			TD0_alpha_arg2_fcol |
			TD0_alpha_sel_arg2 );
		break;

	case GL_ADD:
		if (unit == 0)
			*reg = ( TD0_color_arg2_diffuse |
				 TD0_color_add_add |
				 TD0_color_sel_add | 
				 TD0_alpha_arg2_diffuse |
				 TD0_alpha_sel_arg1);
		else
			*reg = ( TD0_color_arg2_prevstage |
				 TD0_color_alpha_prevstage |
				 TD0_color_add_add |
				 TD0_color_sel_add | 
				 TD0_alpha_arg2_prevstage |
				 TD0_alpha_sel_arg1);
		break;

	case GL_BLEND:
		/* Use a multipass mechanism to do this:
		 */
		mmesa->Fallback |= MGA_FALLBACK_TEXTURE;
		break;
	default:
	}
}



static void mgaUpdateTextureObject( GLcontext *ctx, int unit ) {
	mgaTextureObjectPtr t;
	struct gl_texture_object	*tObj;
	GLuint enabled;
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	GLuint source = mmesa->tmu_source[unit];

	mgaMsg(15,"mgaUpdateTextureState %d\n", unit);
	
	/* disable texturing until it is known to be good */
	mmesa->Setup[MGA_CTXREG_DWGCTL] = 
		(( mmesa->Setup[MGA_CTXREG_DWGCTL] & DC_opcod_MASK ) | 
		 DC_opcod_trap);

	enabled = (ctx->Texture.Enabled>>(source*4))&TEXTURE0_ANY;
	if (enabled != TEXTURE0_2D) {
		if (enabled)
			mmesa->Fallback |= MGA_FALLBACK_TEXTURE;
		return;
	}

	tObj = ctx->Texture.Unit[source].Current;

	if ( !tObj || tObj != ctx->Texture.Unit[source].CurrentD[2] ) 
		return;
	
	/* if the texture object doesn't exist at all (never used or
	   swapped out), create it now, uploading all texture images */

	if ( !tObj->DriverData ) {
		/* clear the current pointer so that texture object can be
		   swapped out if necessary to make room */
		mmesa->CurrentTexObj[source] = NULL;
		mgaCreateTexObj( mmesa, tObj );

		if ( !tObj->DriverData ) {
			mgaMsg( 5, "mgaUpdateTextureState: create failed\n" );
			mmesa->Fallback |= MGA_FALLBACK_TEXTURE;
			return;		/* can't create a texture object */
		}
	}

	/* we definately have a valid texture now */
	mmesa->Setup[MGA_CTXREG_DWGCTL] = 
		(( mmesa->Setup[MGA_CTXREG_DWGCTL] & DC_opcod_MASK ) | 
		 DC_opcod_texture_trap);

	t = (mgaTextureObjectPtr)tObj->DriverData;

	if (t->dirty_images) 
		mmesa->dirty |= (MGA_UPLOAD_TEX0IMAGE << unit);

	mmesa->CurrentTexObj[unit] = t;

	t->Setup[MGA_TEXREG_CTL2] &= ~TMC_dualtex_enable;
	if (ctx->Texture.Enabled == (TEXTURE0_2D|TEXTURE1_2D))
		t->Setup[MGA_TEXREG_CTL2] |= TMC_dualtex_enable;

	t->Setup[MGA_TEXREG_CTL2] &= ~TMC_specen_enable;
	if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
		t->Setup[MGA_TEXREG_CTL2] |= TMC_specen_enable;
       
}






/* The G400 is now programmed quite differently wrt texture environment.
 */
void mgaUpdateTextureState( GLcontext *ctx )
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mmesa->Fallback &= ~MGA_FALLBACK_TEXTURE;

	if (MGA_IS_G400(mmesa)) {
		mgaUpdateTextureObject( ctx, 0 );		
		mgaUpdateTextureStage( ctx, 0 );

		mmesa->Setup[MGA_CTXREG_TDUAL1] = mmesa->Setup[MGA_CTXREG_TDUAL0];

		if (mmesa->multitex) {
			mgaUpdateTextureObject( ctx, 1 );	
			mgaUpdateTextureStage( ctx, 1 );
		}
	} else {
		mgaUpdateTextureObject( ctx, 0 );
		mgaUpdateTextureEnvG200( ctx );
	}

	/* schedule the register writes */
	mmesa->dirty |= MGA_UPLOAD_CTX;
}



/*
============================================================================

Driver functions called directly from mesa

============================================================================
*/

/*
 * mgaTexEnv
 */      
void mgaTexEnv( GLcontext *ctx, GLenum pname, const GLfloat *param ) {
	mgaMsg( 10, "mgaTexEnv( %i )\n", pname );

	if (pname == GL_TEXTURE_ENV_MODE) {
		/* force the texture state to be updated */
		MGA_CONTEXT(ctx)->CurrentTexObj[0] = 0;  
		MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
	}
}

/*
 * mgaTexImage
 */ 
void mgaTexImage( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *tObj, GLint level,
		  GLint internalFormat,
		  const struct gl_texture_image *image ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );	
	mgaTextureObjectPtr t;

	mgaMsg( 10,"mgaTexImage( %p, level %i )\n", tObj, level );
  
 	/* just free the mga texture if it exists, it will be recreated at
	mgaUpdateTextureState time. */  
	t = (mgaTextureObjectPtr) tObj->DriverData;
	if ( t ) {
		/* if this is the current object, it will force an update */
 	 	mgaDestroyTexObj( mmesa, t );
		mmesa->new_state |= MGA_NEW_TEXTURE;
	}
}

/*
 * mgaTexSubImage
 */      
void mgaTexSubImage( GLcontext *ctx, GLenum target,
		     struct gl_texture_object *tObj, GLint level,
		     GLint xoffset, GLint yoffset,
		     GLsizei width, GLsizei height,
		     GLint internalFormat,
		     const struct gl_texture_image *image ) 
{
	mgaContextPtr mmesa = MGA_CONTEXT( ctx );
	mgaTextureObjectPtr t;

	mgaMsg(10,"mgaTexSubImage() Size: %d,%d of %d,%d; Level %d\n",
		width, height, image->Width,image->Height, level);

	t = (mgaTextureObjectPtr) tObj->DriverData;


 	/* just free the mga texture if it exists, it will be recreated at
	mgaUpdateTextureState time. */  
	t = (mgaTextureObjectPtr) tObj->DriverData;
	if ( t ) {
		/* if this is the current object, it will force an update */
 	 	mgaDestroyTexObj( mmesa, t );
		mmesa->new_state |= MGA_NEW_TEXTURE;
	}



#if 0
	/* the texture currently exists, so directly update it */
	mgaUploadSubImage( t, level, xoffset, yoffset, width, height );
#endif
}

/*
 * mgaTexParameter
 * This just changes variables and flags for a state update, which
 * will happen at the next mgaUpdateTextureState
 */
void mgaTexParameter( GLcontext *ctx, GLenum target,
		      struct gl_texture_object *tObj,
		      GLenum pname, const GLfloat *params ) {
	mgaTextureObjectPtr t;

	mgaMsg( 10, "mgaTexParameter( %p, %i )\n", tObj, pname );

	t = (mgaTextureObjectPtr) tObj->DriverData;

	/* if we don't have a hardware texture, it will be automatically
	created with current state before it is used, so we don't have
	to do anything now */
	if ( !t || target != GL_TEXTURE_2D ) {
		return;
	}

	switch (pname) {
	case GL_TEXTURE_MIN_FILTER:
	case GL_TEXTURE_MAG_FILTER:
		mgaSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
		break;

	case GL_TEXTURE_WRAP_S:
	case GL_TEXTURE_WRAP_T:
		mgaSetTexWrapping(t,tObj->WrapS,tObj->WrapT);
		break;
  
	case GL_TEXTURE_BORDER_COLOR:
		mgaSetTexBorderColor(t,tObj->BorderColor);
		break;

	default:
		return;
	}
	/* force the texture state to be updated */
	MGA_CONTEXT(ctx)->CurrentTexObj[0] = NULL;  
	MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
}

/*
 * mgaBindTexture
 */      
void mgaBindTexture( GLcontext *ctx, GLenum target,
		     struct gl_texture_object *tObj ) {

	mgaMsg( 10, "mgaBindTexture( %p )\n", tObj );
		
	/* force the texture state to be updated 
	 */
	MGA_CONTEXT(ctx)->CurrentTexObj[0] = NULL;  
	MGA_CONTEXT(ctx)->new_state |= MGA_NEW_TEXTURE;
}

/*
 * mgaDeleteTexture
 */      
void mgaDeleteTexture( GLcontext *ctx, struct gl_texture_object *tObj ) {

	mgaMsg( 10, "mgaDeleteTexture( %p )\n", tObj );
	
	/* delete our driver data */
	if ( tObj->DriverData ) {
		mgaContextPtr mmesa = MGA_CONTEXT( ctx );
		mgaDestroyTexObj( mmesa,
				  (mgaTextureObjectPtr)(tObj->DriverData) );
		mmesa->new_state |= MGA_NEW_TEXTURE;
	}
}


/* Have to grab the lock to find out if anyone has kicked out our
 * textures.
 */
GLboolean mgaIsTextureResident( GLcontext *ctx, struct gl_texture_object *t ) 
{
	mgaTextureObjectPtr mt;

/*  	LOCK_HARDWARE; */
	mt = (mgaTextureObjectPtr)t->DriverData;
/*  	UNLOCK_HARDWARE; */

	return mt && mt->MemBlock;
}

void mgaDDInitTextureFuncs( GLcontext *ctx )
{
   ctx->Driver.TexEnv = mgaTexEnv;
   ctx->Driver.TexImage = mgaTexImage;
   ctx->Driver.TexSubImage = mgaTexSubImage;
   ctx->Driver.BindTexture = mgaBindTexture;
   ctx->Driver.DeleteTexture = mgaDeleteTexture;
   ctx->Driver.TexParameter = mgaTexParameter;
   ctx->Driver.UpdateTexturePalette = 0;
   ctx->Driver.IsTextureResident = mgaIsTextureResident;
}
