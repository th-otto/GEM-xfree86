/* $TOG: list.c /main/8 1998/02/09 14:13:35 kaleb $ */
/******************************************************************************

Copyright 1993, 1998  The Open Group

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
******************************************************************************/

#include "xsm.h"

List *
ListInit()
{
	List *l;

	l = (List *)XtMalloc(sizeof *l);
	if(!l) return l;
	l->next = l;
	l->prev = l;
	l->thing = NULL;
	return l;
}

List *
ListFirst(l)
List *l;
{
	if(l->next->thing) return l->next;
	else return NULL;
}

List *
ListNext(l)
List *l;
{
	if(l->next->thing) return l->next;
	else return NULL;
}

void
ListFreeAll(l)
List *l;
{
	char *thing;
	List *next;

	next = l->next;
	do {
		l = next;
		next = l->next;
		thing = l->thing;
		XtFree((char *)l);
	} while(thing);
}

void
ListFreeAllButHead(l)
List *l;
{
	List *p, *next;

	p = ListFirst(l);

	while (p)
	{
	    next = ListNext (p);
	    XtFree((char *) p);
	    p = next;
	}

	l->next = l;
	l->prev = l;
}

List *
ListAddFirst(l, v)
List *l;
char *v;
{
	List *e;

	e = (List *)XtMalloc(sizeof *e);
	if(!e) return NULL;

	e->thing = v;
	e->prev = l;
	e->next = e->prev->next;
	e->prev->next = e;
	e->next->prev = e;

	return e;
}

List *
ListAddLast(l, v)
List *l;
char *v;
{
	List *e;

	e = (List *)XtMalloc(sizeof *e);
	if(!e) return NULL;

	e->thing = v;
	e->next = l;
	e->prev = e->next->prev;
	e->prev->next = e;
	e->next->prev = e;

	return e;
}

void
ListFreeOne(e)
List *e;
{
	e->next->prev = e->prev;
	e->prev->next = e->next;
	XtFree((char *)e);
}


Status
ListSearchAndFreeOne(l,thing)
List *l;
char *thing;
{
    	List *p;

	for (p = ListFirst (l); p; p = ListNext (p))
	    if (((char *) p->thing) == (char *) thing)
	    {
		ListFreeOne (p);
		return (1);
	    }

	return (0);
}


int
ListCount(l)
List *l;
{
	int i;
	List *e;

	i = 0;
	for(e = ListFirst(l); e; e = ListNext(e)) i++;

	return i;
}
