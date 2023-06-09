/* $TOG: jdhuff.c /main/6 1998/02/09 16:19:31 kaleb $ */
/* AGE Logic - Oct 15 1995 - Larry Hare */
/* Module jdhuff.c */

/****************************************************************************

Copyright 1993, 1994, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


				NOTICE
                              
This software is being provided by AGE Logic, Inc. under the
following license.  By obtaining, using and/or copying this software,
you agree that you have read, understood, and will comply with these
terms and conditions:

     Permission to use, copy, modify, distribute and sell this
     software and its documentation for any purpose and without
     fee or royalty and to grant others any or all rights granted
     herein is hereby granted, provided that you agree to comply
     with the following copyright notice and statements, including
     the disclaimer, and that the same appears on all copies and
     derivative works of the software and documentation you make.
     
     "Copyright 1993, 1994 by AGE Logic, Inc."
     
     THIS SOFTWARE IS PROVIDED "AS IS".  AGE LOGIC MAKES NO
     REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED.  By way of
     example, but not limitation, AGE LOGIC MAKE NO
     REPRESENTATIONS OR WARRANTIES OF MERCHANTABILITY OR FITNESS
     FOR ANY PARTICULAR PURPOSE OR THAT THE SOFTWARE DOES NOT
     INFRINGE THIRD-PARTY PROPRIETARY RIGHTS.  AGE LOGIC 
     SHALL BEAR NO LIABILITY FOR ANY USE OF THIS SOFTWARE.  IN NO
     EVENT SHALL EITHER PARTY BE LIABLE FOR ANY INDIRECT,
     INCIDENTAL, SPECIAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOSS
     OF PROFITS, REVENUE, DATA OR USE, INCURRED BY EITHER PARTY OR
     ANY THIRD PARTY, WHETHER IN AN ACTION IN CONTRACT OR TORT OR
     BASED ON A WARRANTY, EVEN IF AGE LOGIC LICENSEES
     HEREUNDER HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
     DAMAGES.
    
     The name of AGE Logic, Inc. may not be used in
     advertising or publicity pertaining to this software without
     specific, written prior permission from AGE Logic.

     Title to this software shall at all times remain with AGE
     Logic, Inc.
*****************************************************************************

	Gary Rogers, AGE Logic, Inc., October 1993

****************************************************************************/

/*
 * jdhuff.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines.
 * These routines are invoked via the methods entropy_decode
 * and entropy_decode_init/term.
 */

#include "jinclude.h"
#include "macro.h"


/* Static variables to avoid passing 'round extra parameters */

static decompress_info_ptr dcinfo;

static INT32 get_buffer;	   /* current bit-extraction buffer */
static int bits_left;		   /* # of unused bits in it */
#ifdef XIE_SUPPORTED
static UINT16 restarts_to_go;             /* MCUs left in this restart interval */
static short next_restart_num;            /* # of next RSTn marker (0..7) */
#define XIE_EOB   0xffffffff  /* Return value when input buffer is empty */
#else
static boolean printed_eod;	/* flag to suppress multiple end-of-data msgs */
#endif  /* XIE_SUPPORTED */

LOCAL void
#ifdef XIE_SUPPORTED
#if NeedFunctionPrototypes
fix_huff_tbl (HUFF_TBL * htbl)
#else
fix_huff_tbl (htbl)
	HUFF_TBL * htbl;
#endif	/* NeedFunctionPrototypes */
#else
fix_huff_tbl (HUFF_TBL * htbl)
#endif	/* XIE_SUPPORTED */
/* Compute derived values for a Huffman table */
{
  int p, i, l, si;
  char huffsize[257];
  UINT16 huffcode[257];
  UINT16 code;
  
  /* Figure C.1: make table of Huffman code length for each symbol */
  /* Note that this is in code-length order. */

  p = 0;
  for (l = 1; l <= 16; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  
  /* Figure C.2: generate the codes themselves */
  /* Note that this is in code-length order. */
  
  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    code <<= 1;
    si++;
  }

  /* We don't bother to fill in the encoding tables ehufco[] and ehufsi[], */
  /* since they are not used for decoding. */

  /* Figure F.15: generate decoding tables */

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {
      htbl->valptr[l] = p;	/* huffval[] index of 1st sym of code len l */
      htbl->mincode[l] = huffcode[p]; /* minimum code of length l */
      p += htbl->bits[l];
      htbl->maxcode[l] = huffcode[p-1];	/* maximum code of length l */
    } else {
      htbl->maxcode[l] = -1;
    }
  }
  htbl->maxcode[17] = 0xFFFFFL;	/* ensures huff_DECODE terminates */
}


/*
 * Code for extracting the next N bits from the input stream.
 * (N never exceeds 15 for JPEG data.)
 * This needs to go as fast as possible!
 *
 * We read source bytes into get_buffer and dole out bits as needed.
 * If get_buffer already contains enough bits, they are fetched in-line
 * by the macros get_bits() and get_bit().  When there aren't enough bits,
 * fill_bit_buffer is called; it will attempt to fill get_buffer to the
 * "high water mark", then extract the desired number of bits.  The idea,
 * of course, is to minimize the function-call overhead cost of entering
 * fill_bit_buffer.
 * On most machines MIN_GET_BITS should be 25 to allow the full 32-bit width
 * of get_buffer to be used.  (On machines with wider words, an even larger
 * buffer could be used.)  However, on some machines 32-bit shifts are
 * relatively slow and take time proportional to the number of places shifted.
 * (This is true with most PC compilers, for instance.)  In this case it may
 * be a win to set MIN_GET_BITS to the minimum value of 15.  This reduces the
 * average shift distance at the cost of more calls to fill_bit_buffer.
 */

#ifdef SLOW_SHIFT_32
#define MIN_GET_BITS  15	/* minimum allowable value */
#else
#define MIN_GET_BITS  25	/* max value for 32-bit get_buffer */
#endif

static const int bmask[16] =	/* bmask[n] is mask for n rightmost bits */
  { 0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF };


LOCAL int
#ifdef XIE_SUPPORTED
#if NeedFunctionPrototypes
fill_bit_buffer (int nbits)
#else
fill_bit_buffer (nbits)
	int nbits;
#endif	/* NeedFunctionPrototypes */
#else
fill_bit_buffer (int nbits)
#endif	/* XIE_SUPPORTED */
/* Load up the bit buffer and do get_bits(nbits) */
{
  /* Attempt to load at least MIN_GET_BITS bits into get_buffer. */
  while (bits_left < MIN_GET_BITS) {
    register int c = JGETC(dcinfo);
#ifdef XIE_SUPPORTED
    if (c < 0)
      return(XIE_EOB);
#endif	/* XIE_SUPPORTED */
    
    /* If it's 0xFF, check and discard stuffed zero byte */
    if (c == 0xFF) {
      int c2 = JGETC(dcinfo);
#ifdef XIE_SUPPORTED
      if (c2 < 0)
        return(XIE_EOB);
#endif	/* XIE_SUPPORTED */
      if (c2 != 0) {
	/* Oops, it's actually a marker indicating end of compressed data. */
	/* Better put it back for use later */
	JUNGETC(c2,dcinfo);
	JUNGETC(c,dcinfo);
	/* There should be enough bits still left in the data segment; */
	/* if so, just break out of the while loop. */
	if (bits_left >= nbits)
	  break;
	/* Uh-oh.  Report corrupted data to user and stuff zeroes into
	 * the data stream, so we can produce some kind of image.
	 * Note that this will be repeated for each byte demanded for the
	 * rest of the segment; this is a bit slow but not unreasonably so.
	 * The main thing is to avoid getting a zillion warnings, hence:
	 */
#ifndef XIE_SUPPORTED
	if (! printed_eod) {
	  WARNMS(dcinfo->emethods, "Corrupt JPEG data: premature end of data segment");
	  printed_eod = TRUE;
	}
#endif  /* XIE_SUPPORTED */
	c = 0;			/* insert a zero byte into bit buffer */
      }
    }

    /* OK, load c into get_buffer */
    get_buffer = (get_buffer << 8) | c;
    bits_left += 8;
  }

  /* Having filled get_buffer, extract desired bits (this simplifies macros) */
  bits_left -= nbits;
  return ((int) (get_buffer >> bits_left)) & bmask[nbits];
}


/* Macros to make things go at some speed! */
/* NB: parameter to get_bits should be simple variable, not expression */

#define get_bits(nbits) \
	(bits_left >= (nbits) ? \
	 ((int) (get_buffer >> (bits_left -= (nbits)))) & bmask[nbits] : \
	 fill_bit_buffer(nbits))

#define get_bit() \
	(bits_left ? \
	 ((int) (get_buffer >> (--bits_left))) & 1 : \
	 fill_bit_buffer(1))


/* Figure F.16: extract next coded symbol from input stream */
  
INLINE
LOCAL int
#ifdef XIE_SUPPORTED
#if NeedFunctionPrototypes
huff_DECODE (HUFF_TBL * htbl)
#else
huff_DECODE (htbl)
	HUFF_TBL * htbl;
#endif	/* NeedFunctionPrototypes */
#else
huff_DECODE (HUFF_TBL * htbl)
#endif	/* XIE_SUPPORTED */
{
  register int l;
  register INT32 code;
  
#ifdef XIE_SUPPORTED
  register int j;
  
  if ((code = get_bit()) == XIE_EOB)
    return(XIE_EOB);
  l = 1;
  while (code > htbl->maxcode[l]) {
    if ((j = get_bit()) == XIE_EOB)
      return(XIE_EOB);
    code = (code << 1) | j;
    l++;
  }
#else
  code = get_bit();
  l = 1;
  while (code > htbl->maxcode[l]) {
    code = (code << 1) | get_bit();
    l++;
  }
#endif	/* XIE_SUPPORTED */

  /* With garbage input we may reach the sentinel value l = 17. */

  if (l > 16) {
#ifndef XIE_SUPPORTED
    WARNMS(dcinfo->emethods, "Corrupt JPEG data: bad Huffman code");
#endif	/* XIE_SUPPORTED */
    return 0;			/* fake a zero as the safest result */
  }

  return htbl->huffval[ htbl->valptr[l] + ((int) (code - htbl->mincode[l])) ];
}


/* Figure F.12: extend sign bit */

#define huff_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };


/*
 * Initialize for a Huffman-compressed scan.
 * This is invoked after reading the SOS marker.
 */

#ifdef XIE_SUPPORTED
METHODDEF int
#if NeedFunctionPrototypes
huff_decoder_init (decompress_info_ptr cinfo)
#else
huff_decoder_init (cinfo)
	decompress_info_ptr cinfo;
#endif	/* NeedFunctionPrototypes */
#else
METHODDEF void
huff_decoder_init (decompress_info_ptr cinfo)
#endif	/* XIE_SUPPORTED */
{
  short ci;
  jpeg_component_info * compptr;

  /* Initialize static variables */
  dcinfo = cinfo;
#ifdef XIE_SUPPORTED
  bits_left = 0;
  cinfo->XIEbits_left = bits_left;
  get_buffer = 0;
  cinfo->XIEget_buffer = get_buffer;
  cinfo->printed_eod = FALSE;
#else  
  bits_left = 0;
  printed_eod = FALSE;
#endif	/* XIE_SUPPORTED */


  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    /* Make sure requested tables are present */
    if (cinfo->dc_huff_tbl_ptrs[compptr->dc_tbl_no] == NULL ||
	cinfo->ac_huff_tbl_ptrs[compptr->ac_tbl_no] == NULL)
#ifdef XIE_SUPPORTED
      return(XIE_ERR);
#else
      ERREXIT(cinfo->emethods, "Use of undefined Huffman table");
#endif	/* XIE_SUPPORTED */
    /* Compute derived values for Huffman tables */
    /* We may do this more than once for same table, but it's not a big deal */
    fix_huff_tbl(cinfo->dc_huff_tbl_ptrs[compptr->dc_tbl_no]);
    fix_huff_tbl(cinfo->ac_huff_tbl_ptrs[compptr->ac_tbl_no]);
    /* Initialize DC predictions to 0 */
    cinfo->last_dc_val[ci] = 0;
  }

  /* Initialize restart stuff */
  cinfo->restarts_to_go = cinfo->restart_interval;
  cinfo->next_restart_num = 0;
#ifdef XIE_SUPPORTED
  return(0);
#endif	/* XIE_SUPPORTED */
}


/*
 * Check for a restart marker & resynchronize decoder.
 */

#ifdef XIE_SUPPORTED
LOCAL int
#if NeedFunctionPrototypes
process_restart (decompress_info_ptr cinfo)
#else
process_restart (cinfo)
	decompress_info_ptr cinfo;
#endif	/* NeedFunctionPrototypes */
#else
LOCAL void
process_restart (decompress_info_ptr cinfo)
#endif	/* XIE_SUPPORTED */
{
  int c, nbytes;
  short ci;

  /* Throw away any unused bits remaining in bit buffer */
  nbytes = bits_left / 8;	/* count any full bytes loaded into buffer */
  bits_left = 0;
#ifdef XIE_SUPPORTED
  cinfo->printed_eod = FALSE;		/* next segment can get another warning */
#else  
  printed_eod = FALSE;		        /* next segment can get another warning */
#endif	/* XIE_SUPPORTED */

  /* Scan for next JPEG marker */
  do {
    do {			/* skip any non-FF bytes */
      nbytes++;
      c = JGETC(cinfo);
#ifdef XIE_SUPPORTED
      if (c < 0)
        return(XIE_EOB);
#endif	/* XIE_SUPPORTED */
    } while (c != 0xFF);
    do {			/* skip any duplicate FFs */
      /* we don't increment nbytes here since extra FFs are legal */
      c = JGETC(cinfo);
#ifdef XIE_SUPPORTED
      if (c < 0)
        return(XIE_EOB);
#endif	/* XIE_SUPPORTED */
    } while (c == 0xFF);
  } while (c == 0);		/* repeat if it was a stuffed FF/00 */

#ifndef XIE_SUPPORTED
  if (nbytes != 1)
    WARNMS2(cinfo->emethods,
	    "Corrupt JPEG data: %d extraneous bytes before marker 0x%02x",
	    nbytes-1, c);
#endif	/* XIE_SUPPORTED */

  if (c != (RST0 + cinfo->next_restart_num)) {
    /* Uh-oh, the restart markers have been messed up too. */
    /* Let the file-format module try to figure out how to resync. */
    (*cinfo->methods->resync_to_restart) (cinfo, c);
#ifdef XIE_SUPPORTED
  }
#else
  } else
    TRACEMS1(cinfo->emethods, 2, "RST%d", cinfo->next_restart_num);
#endif	/* XIE_SUPPORTED */

#ifndef XIE_SUPPORTED
  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    cinfo->last_dc_val[ci] = 0;

  /* Update restart state */
  cinfo->restarts_to_go = cinfo->restart_interval;
  cinfo->next_restart_num = (cinfo->next_restart_num + 1) & 7;
#else
  restarts_to_go = cinfo->restart_interval;
  next_restart_num = (next_restart_num + 1) & 7;
  return(0);
#endif	/* XIE_SUPPORTED */
}


/* ZAG[i] is the natural-order position of the i'th element of zigzag order.
 * If the incoming data is corrupted, huff_decode_mcu could attempt to
 * reference values beyond the end of the array.  To avoid a wild store,
 * we put some extra zeroes after the real entries.
 */

static const short ZAG[DCTSIZE2+16] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63,
  0,  0,  0,  0,  0,  0,  0,  0, /* extra entries in case k>63 below */
  0,  0,  0,  0,  0,  0,  0,  0
};


/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 * This routine also handles quantization descaling and zigzag reordering
 * of coefficient values.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU_data[i].  WE ASSUME THIS AREA HAS BEEN ZEROED BY THE CALLER.
 * (Wholesale zeroing is usually a little faster than retail...)
 */

#ifdef XIE_SUPPORTED
METHODDEF int
#if NeedFunctionPrototypes
huff_decode_mcu (decompress_info_ptr cinfo, JBLOCKROW *MCU_data)
#else
huff_decode_mcu (cinfo, MCU_data)
	decompress_info_ptr cinfo;
	JBLOCKROW *MCU_data;
#endif	/* NeedFunctionPrototypes */
#else
METHODDEF void
huff_decode_mcu (decompress_info_ptr cinfo, JBLOCKROW *MCU_data)
#endif	/* XIE_SUPPORTED */
{
  register int s, k, r;
  short blkn, ci;
  register JBLOCKROW block;
  register QUANT_TBL_PTR quanttbl;
  HUFF_TBL *dctbl;
  HUFF_TBL *actbl;
  jpeg_component_info * compptr;

#ifdef XIE_SUPPORTED
  JCOEF last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each comp */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    last_dc_val[ci] = cinfo->last_dc_val[ci];
      
  dcinfo = cinfo;
  bits_left = cinfo->XIEbits_left;
  get_buffer = cinfo->XIEget_buffer;
  restarts_to_go = cinfo->restarts_to_go;
  next_restart_num = cinfo->next_restart_num;
#endif	/* XIE_SUPPORTED */

  /* Account for restart interval, process restart marker if needed */
  if (cinfo->restart_interval) {
    if (cinfo->restarts_to_go == 0)
#ifdef XIE_SUPPORTED
    {
      s = process_restart(cinfo);
      if (s == XIE_EOB)
        return(XIE_EOB);
      for (ci = 0; ci < cinfo->comps_in_scan; ci++)
        last_dc_val[ci] = 0;
    }
    restarts_to_go--;
#else    
      process_restart(cinfo);
    cinfo->restarts_to_go--;
#endif	/* XIE_SUPPORTED */
  }

  /* Outer loop handles each block in the MCU */

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    quanttbl = cinfo->quant_tbl_ptrs[compptr->quant_tbl_no];
    actbl = cinfo->ac_huff_tbl_ptrs[compptr->ac_tbl_no];
    dctbl = cinfo->dc_huff_tbl_ptrs[compptr->dc_tbl_no];

    /* Decode a single block's worth of coefficients */

    /* Section F.2.2.1: decode the DC coefficient difference */
    s = huff_DECODE(dctbl);
#ifdef XIE_SUPPORTED
    if (s == XIE_EOB) {
      return(XIE_EOB);
    }
#endif	/* XIE_SUPPORTED */
    if (s) {
      r = get_bits(s);
#ifdef XIE_SUPPORTED
      if (r == XIE_EOB) {
        return(XIE_EOB);
      }
#endif	/* XIE_SUPPORTED */
      s = huff_EXTEND(r, s);
    }

    /* Convert DC difference to actual value, update last_dc_val */
#ifdef XIE_SUPPORTED
    s += last_dc_val[ci];
    last_dc_val[ci] = (JCOEF) s;
#else
    s += cinfo->last_dc_val[ci];
    cinfo->last_dc_val[ci] = (JCOEF) s;
#endif	/* XIE_SUPPORTED */
    /* Descale and output the DC coefficient (assumes ZAG[0] = 0) */
    (*block)[0] = (JCOEF) (((JCOEF) s) * quanttbl[0]);
    
    /* Section F.2.2.2: decode the AC coefficients */
    /* Since zero values are skipped, output area must be zeroed beforehand */
    for (k = 1; k < DCTSIZE2; k++) {
      r = huff_DECODE(actbl);
#ifdef XIE_SUPPORTED
      if (r == XIE_EOB) {
        return(XIE_EOB);
      }
#endif	/* XIE_SUPPORTED */
      
      s = r & 15;
      r = r >> 4;
      
      if (s) {
        k += r;
        r = get_bits(s);
#ifdef XIE_SUPPORTED
    if (r == XIE_EOB) {
      return(XIE_EOB);
    }
#endif	/* XIE_SUPPORTED */
	s = huff_EXTEND(r, s);
	/* Descale coefficient and output in natural (dezigzagged) order */
	(*block)[ZAG[k]] = (JCOEF) (((JCOEF) s) * quanttbl[k]);
      } else {
    	if (r != 15)
    	  break;
    	k += 15;
      }
    }
  }
#ifdef XIE_SUPPORTED
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    cinfo->last_dc_val[ci] = last_dc_val[ci];
  cinfo->XIEbits_left = bits_left;
  cinfo->XIEget_buffer = get_buffer;
  cinfo->restarts_to_go = restarts_to_go;
  cinfo->next_restart_num = next_restart_num;
  return(0);
#endif	/* XIE_SUPPORTED */
}


/*
 * Finish up at the end of a Huffman-compressed scan.
 */

#ifndef XIE_SUPPORTED
METHODDEF void
#ifdef XIE_SUPPORTED
#if NeedFunctionPrototypes
huff_decoder_term (decompress_info_ptr cinfo)
#else
huff_decoder_term (cinfo)
	decompress_info_ptr cinfo;
#endif	/* NeedFunctionPrototypes */
#else
huff_decoder_term (decompress_info_ptr cinfo)
#endif	/* XIE_SUPPORTED */
{
  /* No work needed */
}
#endif   /* XIE_SUPPORTED */

/*
 * The method selection routine for Huffman entropy decoding.
 */

GLOBAL void
#ifdef XIE_SUPPORTED
#if NeedFunctionPrototypes
jseldhuffman (decompress_info_ptr cinfo)
#else
jseldhuffman (cinfo)
	decompress_info_ptr cinfo;
#endif	/* NeedFunctionPrototypes */
#else
jseldhuffman (decompress_info_ptr cinfo)
#endif	/* XIE_SUPPORTED */
{
  if (! cinfo->arith_code) {
    cinfo->methods->entropy_decode_init = huff_decoder_init;
    cinfo->methods->entropy_decode = huff_decode_mcu;
#ifndef XIE_SUPPORTED	    
    cinfo->methods->entropy_decode_term = huff_decoder_term;
#endif   /* XIE_SUPPORTED */    
  }
}
