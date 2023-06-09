/* $TOG: sched.h /main/5 1998/02/09 16:18:15 kaleb $ */
/**** module sched.h ****/
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
******************************************************************************

	sched.h -- DDXIE machine independent scheduler definitions

	Robert NC Shelley -- AGE Logic, Inc.  May 1993

******************************************************************************/
/* $XFree86: xc/programs/Xserver/XIE/mixie/include/sched.h,v 1.3 1998/10/25 12:47:57 dawes Exp $ */

#ifndef _XIEH_SCHED
#define _XIEH_SCHED

/* scheduler interface
 */
struct _petex;
typedef bandMsk	(*xieBandProc)(floDefPtr flo, struct _petex *pet);
typedef struct _schedvec {
  xieBoolProc	execute;
  xieBandProc	runnable;
} schedVecRec;

/* call scheduler to execute until out of data;
 * returns Bool: true if flo still active
 */
#define Execute(flo,import_pet) \
		(*flo->schedVec->execute)(flo,import_pet)

/* detect execution suspension/resumption
 * returns Bool: true if scheduler has exited to core X since last checked
 * (useful for determining if fresh lookups are needed for core X resources)
 */
#define Resumed(flo,pet) \
		(pet->schedCnt == flo->floTex->exitCnt ? FALSE \
                 : ((pet->schedCnt = flo->floTex->exitCnt), TRUE))

/* toggle the band's ready bit if "available >= threshold" status has changed
 * and return the updated ready mask;
 * if the ready bit doesn't toggle, return NO_BANDS
 */
#define CheckSrcReady(bnd,bmsk) \
		(bnd->receptor->ready & (bmsk) \
		 ? (bnd->available < bnd->threshold \
		    ? (bnd->receptor->ready &= ~(bmsk)) : NO_BANDS) \
		 : (bnd->available >= bnd->threshold \
		    ? (bnd->receptor->ready |=  (bmsk)) : NO_BANDS))

/* try to schedule the specified element based on new data arriving for:
 * 			receptorPtr rcp, bandPtr bnd, and bandMsk bmsk
 * nothing is returned, but pet->scheduled indicates which bands are runnable
 */
#define Schedule(flo,pet,rcp,bnd,bmsk) \
		{ bandMsk r = CheckSrcReady(bnd,bmsk) & rcp->attend; \
		  if(r && !pet->scheduled) { \
		    if(!pet->inSync) { \
		      if(!pet->bandSync || r == (rcp->active & rcp->attend)) {\
		        pet->scheduled |= r; \
			InsertMember(pet, &flo->floTex->schedHead); \
		      } \
		    } else if((r = (*flo->schedVec->runnable)(flo,pet)) != 0) { \
		      pet->scheduled = r; \
		      InsertMember(pet,&flo->floTex->schedHead); \
		} } }
#endif /* end _XIEH_SCHED */
