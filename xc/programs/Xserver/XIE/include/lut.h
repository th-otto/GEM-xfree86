/* $TOG: lut.h /main/5 1998/02/10 10:27:50 kaleb $ */
/**** module lut.h ****/
/******************************************************************************

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
*******************************************************************************

	lut.h: contains lut specific definitions

	Dean Verheiden -- AGE Logic, Inc. April 1993

******************************************************************************/

#ifndef _XIEH_LUT
#define _XIEH_LUT

#include <flodata.h>

/*
 *  LUT resource definition
 */

typedef struct _lutformat {
  xieTypOrientation	bandOrder; 	/* Lut order */
  CARD8			pad[3];
  CARD32		length;		/* Input quantization levels 	*/
  CARD32		level;		/* Output quantization levels 	*/
} lutFormatRec, *lutFormatPtr;

typedef struct _lut {
  xieTypLUT		ID;
  CARD16		refCnt;
  CARD8			lutCnt;	
  lutFormatRec		format[xieValMaxBands];
  stripLstRec		strips[xieValMaxBands]; /* lists of image strips */
} lutRec, *lutPtr;

/*
 *  Macros for working with Luts
 *	LutPitch(levels) - returns size of single lut array entry in bytes.
 */

#define LutPitch(Levels)                \
	(((Levels) == 0) ? 4 :          \
	 (((Levels) > (1<<16)) ? 4 :    \
	  (((Levels) > (1<<8)) ? 2 : 1)))   

#endif
/* end _XIEH_LUT */
