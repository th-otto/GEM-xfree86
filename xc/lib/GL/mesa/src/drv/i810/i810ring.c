#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "mm.h"
#include "i810dd.h"
#include "i810lib.h"
#include "i810state.h"


#include "i810dma.h"

static void 
I810PrintErrorState( void )
{
   fprintf(stderr,  "pgetbl_ctl: 0x%x pgetbl_err: 0x%x\n", 
	   INREG(PGETBL_CTL),
	   INREG(PGE_ERR));

   fprintf(stderr,  "ipeir: %x iphdr: %x\n", 
	   INREG(IPEIR),
	   INREG(IPEHR));

   fprintf(stderr,  "LP ring tail: %x head: %x len: %x start %x\n",
	   INREG(LP_RING + RING_TAIL),
	   INREG(LP_RING + RING_HEAD) & HEAD_ADDR,
	   INREG(LP_RING + RING_LEN),
	   INREG(LP_RING + RING_START));

   fprintf(stderr,  "eir: %x esr: %x emr: %x\n",
	   INREG16(EIR),
	   INREG16(ESR),
	   INREG16(EMR));

   fprintf(stderr,  "instdone: %x instpm: %x\n",
	   INREG16(INST_DONE),
	   INREG8(INST_PM));

   fprintf(stderr,  "memmode: %x instps: %x\n",
	   INREG(MEMMODE),
	   INREG(INST_PS));

   fprintf(stderr,  "hwstam: %x ier: %x imr: %x iir: %x\n",
	   INREG16(HWSTAM),
	   INREG16(IER),
	   INREG16(IMR),
	   INREG16(IIR));
}




static int
I810USec( void ) 
{
   struct timeval tv;
   struct timezone tz;	
   gettimeofday( &tv, &tz );
   return (tv.tv_sec & 2047) * 1000000 + tv.tv_usec;
}



void
_I810RefreshLpRing( i810ContextPtr imesa, int update )
{
	struct i810_ring_buffer *ring = &(i810glx.LpRing);
	
	ring->head = INREG(LP_RING + RING_HEAD) & HEAD_ADDR;
	ring->tail = INREG(LP_RING + RING_TAIL);
	ring->space = ring->head - (ring->tail+8);

	if (ring->space < 0) {
		ring->space += ring->mem.Size;
		if (update)
			imesa->sarea->lastWrap = ++imesa->sarea->ringAge;
	}
}	

int
_I810WaitLpRing( i810ContextPtr imesa, int n, int timeout_usec )
{
	struct i810_ring_buffer *ring = &(i810glx.LpRing);
	int iters = 0;
	int startTime = 0;
	int curTime = 0;
   
	if (timeout_usec == 0)
		timeout_usec = 15000000;

	if (I810_DEBUG & DEBUG_VERBOSE_API)
		fprintf(stderr, "I810WaitLpRing %d\n", n);

	while (ring->space < n) 
	{
		int i;

		ring->head = INREG(LP_RING + RING_HEAD) & HEAD_ADDR;
		ring->space = ring->head - (ring->tail+8);

		if (ring->space < 0) {
			imesa->sarea->lastWrap = ++imesa->sarea->ringAge;
			ring->space += ring->mem.Size;
		}
      
		iters++;
		curTime = I810USec();
		if ( startTime == 0 || curTime < startTime /*wrap case*/) {
			startTime = curTime;
		} else if ( curTime - startTime > timeout_usec ) { 
			I810PrintErrorState();
			fprintf(stderr, "space: %d wanted %d\n", 
				ring->space, n );
			UNLOCK_HARDWARE(imesa);
			exit(1);
		}

		for (i = 0 ; i < 2000 ; i++)
			;
	}

	return iters;
}

int
_I810Sync( i810ContextPtr imesa ) 
{
	int rv;

	if (I810_DEBUG & DEBUG_VERBOSE_API)
		fprintf(stderr, "I810Sync\n");
   
	/* Send a flush instruction and then wait till the ring is empty.
	 * This is stronger than waiting for the blitter to finish as it also
	 * flushes the internal graphics caches.
	 */
	{
		BEGIN_LP_RING( imesa, 2 );   
		OUT_RING( INST_PARSER_CLIENT | INST_OP_FLUSH );
		OUT_RING( 0 );		/* pad to quadword */
		ADVANCE_LP_RING();
	}


	i810glx.LpRing.synced = 1;	/* ?? */

	rv =  _I810WaitLpRing( imesa, i810glx.LpRing.mem.Size - 8, 0 );	
	imesa->sarea->lastSync = ++imesa->sarea->ringAge;

	return rv;
}




