/* $TOG: XDelAssoc.c /main/7 1998/02/06 16:14:31 kaleb $ */
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
/* $XFree86: xc/lib/oldX/XDelAssoc.c,v 1.2 1999/05/23 06:33:34 dawes Exp $ */

#include "Xlibint.h"
#include "X10.h"

/*
 * XDeleteAssoc - Delete an association in an XAssocTable keyed on
 * an XId.  An association may be removed only once.  Redundant
 * deletes are meaningless (but cause no problems).
 */
int
XDeleteAssoc(dpy, table, x_id)
        register Display *dpy;
	register XAssocTable *table;
	register XID x_id;
{
	int hash;
	register XAssoc *bucket;
	register XAssoc *Entry;

	/* Hash the XId to get the bucket number. */
	hash = x_id & (table->size - 1);
	/* Look up the bucket to get the entries in that bucket. */
	bucket = &table->buckets[hash];
	/* Get the first entry in the bucket. */
	Entry = bucket->next;

	/* Scan through the entries in the bucket for the right XId. */
	for (; Entry != bucket; Entry = Entry->next) {
		if (Entry->x_id == x_id) {
			/* We have the right XId. */
			if (Entry->display == dpy) {
				/* We have the right display. */
				/* We have the right entry! */
				/* Remove it from the queue and */
				/* free the entry. */
				Entry->prev->next = Entry->next;
				Entry->next->prev = Entry->prev;
				Xfree((char *)Entry);
				return 0;
			}
			/* Oops, identical XId's on different displays! */
			continue;
		}
		if (Entry->x_id > x_id) {
			/* We have gone past where it should be. */
			/* It is apparently not in the table. */
			return 0;
		}
	}
	/* It is apparently not in the table. */
	return 0;
}
