/* $TOG: XDestAssoc.c /main/6 1998/02/06 16:14:37 kaleb $ */
/*

Copyright 1985, 1998  The Open Group.

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

*/
/* $XFree86: xc/lib/oldX/XDestAssoc.c,v 1.2 1999/05/23 06:33:34 dawes Exp $ */

#include "Xlibint.h"
#include "X10.h"

/*
 * XDestroyAssocTable - Destroy (free the memory associated with)
 * an XAssocTable.  
 */
int
XDestroyAssocTable(table)
	register XAssocTable *table;
{
	register int i;
	register XAssoc *bucket;
	register XAssoc *Entry, *entry_next;

	/* Free the buckets. */
	for (i = 0; i < table->size; i++) {
		bucket = &table->buckets[i];
		for (
	        	Entry = bucket->next;
			Entry != bucket;
			Entry = entry_next
		) {
		        entry_next = Entry->next;
			Xfree((char *)Entry);
		}
	}

	/* Free the bucket array. */
	Xfree((char *)table->buckets);

	/* Free the table. */
	Xfree((char *)table);
	return 0;
}

