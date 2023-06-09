/* $TOG: dg4.c /main/6 1998/02/09 16:13:57 kaleb $ */
/* AGE Logic - Oct 15 1995 - Larry Hare */
/**** module fax/g4.c ****/
/******************************************************************************

Copyright 1993, 1994,1998  The Open Group

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
  
	fax/g4.c -- DDXIE G4 fax decode technique/element
  
	Ben Fahy -- AGE Logic, Inc. May, 1993
  
*****************************************************************************/


#define _G4
#include "fax.h"
#include "faxint.h"
#include "bits.h"

#include <servermd.h>
	/* pick up the BITMAP_BIT_ORDER from Core X*/


/**********************************************************************/
int decode_g4(state)
FaxState *state;
{
register int bitpos;
register unsigned char *byteptr;
register unsigned char *endptr;
register int a0_color;
register int a0_pos;
register int goal;
register int mode;	/* could overload goal, but will resist */
register int length;
register int width;

int 	lines_found=0;
int	n_old_trans;
int	n_new_trans;
int	*old_trans;
int	*new_trans;
int 	a0a1,a1a2;
int 	length_acc=0;
int 	last_b1_idx=0;
int 	b1_pos,b2_pos;
int 	rl;

	if (state == (FaxState *)NULL)
		return(-1);

	/* set up initial bitstream for the very first strip */
	if (!state->bits.started) {
	    if (state->strip_state != StripStateNew) {
		state->decoder_done = FAX_DECODE_DONE_ErrorBadStripper;
		return(-1);
	    }
	    state->bits.byteptr = (unsigned char *)state->strip;
	    state->bits.endptr  = state->bits.byteptr + state->strip_size-4;
	       /* we will panic with four bytes to go */

	    state->bits.bitpos = 0;
	    state->bits.started = 1;
	}

	localize_state(state);

	if (state->magic_needs) 
	    finish_magic(state->final);
		/* a magic strip was waiting for 1st word of next strip */

/***	Main Decoding Loop	***/
	while(1) {
	  switch(goal) {
	  case FAX_GOAL_StartNewLine:

	    /* if any data produced by previous states, write it out */
	    if (lines_found >= state->nl_sought) 
		    save_state_and_return(state);

	    /* normal line initialization stuff */
	    a0_pos   = -1;
	    a0_color = WHITE;
	    goal     = FAX_GOAL_DetermineMode;
	    reset_transitions();
	    break;
	  case FAX_GOAL_DetermineMode:
	    get_mode_and_length(mode,length,byteptr,bitpos,endptr); 
	    goal = mode;
		/* our goal is now to handle whatever mode we're in! */

	    if (mode != FAX_MODE_Unknown) 
	         adjust_bitstream_8(length,byteptr,bitpos,endptr);
	    break;

	  case  FAX_GOAL_HandleHoriz: 
	    length_acc=0;
	    goal = FAX_GOAL_AccumulateA0A1;
		/* in case I run out of data while getting a0-a1 distance */


	  case FAX_GOAL_AccumulateA0A1:
	    get_a0a1(FAX_GOAL_RecordA0A1);
		/* If we have to return for data before getting the 	*/
		/* whole a0a1 distance, we want to return to the a0a1	*/
		/* accumulate state when we reenter the decoder. If	*/
		/* adjusting the bitstream forces us to return after	*/
		/* getting the last part of the a0a1 distance, then	*/
		/* we want to jump to state FAX_GOAL_RecordA0A1 when	*/
		/* we reenter the decoder.				*/

	  case  FAX_GOAL_RecordA0A1:
	    a0a1 = length_acc;
	    length_acc=0;
	    goal = FAX_GOAL_AccumulateA1A2;
		/* in case I run out of data while getting a1-a2 distance */

	  case  FAX_GOAL_AccumulateA1A2:
	    get_a1a2(FAX_GOAL_FinishHoriz);
		/* If we have to return for data before getting the 	*/
		/* whole a1a2 distance, we want to return to the a1a2	*/
		/* accumulate state when we reenter the decoder. If	*/
		/* adjusting the bitstream forces us to return after	*/
		/* getting the last part of the a1a2 distance, then	*/
		/* we want to jump to state FAX_GOAL_FinishHoriz when	*/
		/* we reenter the decoder.				*/

	  case  FAX_GOAL_FinishHoriz:
	    a1a2 = length_acc;
	    if (a0_pos < 0) {
		/* at start of line, a0a1 is the number of white pixels,  */
		/* which is also the index on the line where white->black */
		new_trans[n_new_trans++] = a0a1;
		new_trans[n_new_trans] = a0_pos = a0a1 + a1a2;
		++n_new_trans;
	    } else {
		/* in middle of the line, a0a1 is run-length, so
		 * a1_pos = a0_pos + a0a1, a2_pos = a1_pos + a1a2
		 *
		 *  0  1  2  3  4  5  6  7  8  9  A  B  C  D 
		 *    |w |w |w |b |b |b |b |b |w |  |  |  |  |
		 *     a0       a1             a2
		 */ 
		new_trans[n_new_trans++] = a0_pos + a0a1;
		new_trans[n_new_trans]   = a0_pos = a0_pos + a0a1 + a1a2;
		++n_new_trans;
	    }
	    if (a0_pos >= width) {
		if (a0_pos > width) {
		   state->decoder_done = FAX_DECODE_DONE_ErrorPastWidth;
		   save_state_and_return(state);
		}
		goal = FAX_GOAL_StartNewLine;
		FlushLineData();
		break;
	    }
	    else 
	        goal = FAX_GOAL_DetermineMode;
	    break;

	  case FAX_MODE_Pass:
	    if (!n_old_trans) {	/* line above all white */
		state->decoder_done = FAX_DECODE_DONE_ErrorPassAboveAllWhite;
		save_state_and_return(state);
	    }
	    find_b2pos(a0_pos,a0_color,n_old_trans,old_trans);

	    a0_pos = b2_pos;
	    if (a0_pos < 0) {
		FlushLineData();
	       	goal = FAX_GOAL_StartNewLine;
	       	break;
	    }
	    if (a0_pos >= width) {
		state->decoder_done = FAX_DECODE_DONE_ErrorBadA0pos;
		save_state_and_return(state);
	    }
	    goal = FAX_GOAL_DetermineMode;
	    break;

	  case FAX_GOAL_HandleVertL3:
	  case FAX_GOAL_HandleVertL2:
	  case FAX_GOAL_HandleVertL1:
	  case FAX_GOAL_HandleVert0:
	  case FAX_GOAL_HandleVertR1:
	  case FAX_GOAL_HandleVertR2:
	  case FAX_GOAL_HandleVertR3:
	    if (n_old_trans) {	/* line above not all white */
		find_b1pos(a0_pos,a0_color,n_old_trans,old_trans);

		if (b1_pos < 0) {
		   if (goal > FAX_GOAL_HandleVert0) {
		     state->decoder_done = FAX_DECODE_DONE_ErrorBadGoal;
		     save_state_and_return(state);
		   }
		   b1_pos = width;
		}
	    } else {		
	       /* line above was all white. Since b1 was first non-white */
	       /* b1 is imaginary transition off right edge.		 */
		b1_pos = width;
	    }
	    /* ok, we are guaranteed that 0 <= b1_pos <= width */
	    /* set a0_pos = a1_pos,  which is relative to b1_pos */
	    a0_pos   = b1_pos + (goal-FAX_GOAL_HandleVert0);
	    a0_color = 1-a0_color;
	    new_trans[n_new_trans++] = a0_pos > 0 ? a0_pos : 0;
	    if (a0_pos >= width) {
	        FlushLineData();
	        goal = FAX_GOAL_StartNewLine;
	    }
	    else  {
		/* not at eol yet */
		goal = FAX_GOAL_DetermineMode;
	    }
	    break;

	  default:
	    state->decoder_done = FAX_DECODE_DONE_ErrorBadGoal;
	    save_state_and_return(state);
	  break;
	  }  /* end of switch */
	}
/***	End, Main Decoding Loop	***/
}
/**** module fax/g4.c ****/
