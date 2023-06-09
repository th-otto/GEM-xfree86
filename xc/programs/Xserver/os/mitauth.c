/* $TOG: mitauth.c /main/12 1998/02/09 15:12:34 kaleb $ */
/*

Copyright 1988, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/os/mitauth.c,v 1.3 1998/10/11 11:23:44 dawes Exp $ */

/*
 * MIT-MAGIC-COOKIE-1 authorization scheme
 * Author:  Keith Packard, MIT X Consortium
 */

#include "X.h"
#include "os.h"
#include "osdep.h"
#include "dixstruct.h"

static struct auth {
    struct auth	*next;
    unsigned short	len;
    char	*data;
    XID		id;
} *mit_auth;

int
MitAddCookie (
    unsigned short	data_length,
    char		*data,
    XID			id)
{
    struct auth	*new;

    new = (struct auth *) xalloc (sizeof (struct auth));
    if (!new)
	return 0;
    new->data = (char *) xalloc ((unsigned) data_length);
    if (!new->data) {
	xfree(new);
	return 0;
    }
    new->next = mit_auth;
    mit_auth = new;
    memmove(new->data, data, (int) data_length);
    new->len = data_length;
    new->id = id;
    return 1;
}

XID
MitCheckCookie (
    unsigned short	data_length,
    char		*data,
    ClientPtr		client,
    char		**reason)
{
    struct auth	*auth;

    for (auth = mit_auth; auth; auth=auth->next) {
        if (data_length == auth->len &&
	   memcmp (data, auth->data, (int) data_length) == 0)
	    return auth->id;
    }
    *reason = "Invalid MIT-MAGIC-COOKIE-1 key";
    return (XID) -1;
}

int
MitResetCookie (void)
{
    struct auth	*auth, *next;

    for (auth = mit_auth; auth; auth=next) {
	next = auth->next;
	xfree (auth->data);
	xfree (auth);
    }
    mit_auth = 0;
    return 0;
}

XID
MitToID (
	unsigned short	data_length,
	char		*data)
{
    struct auth	*auth;

    for (auth = mit_auth; auth; auth=auth->next) {
	if (data_length == auth->len &&
	    memcmp (data, auth->data, data_length) == 0)
	    return auth->id;
    }
    return (XID) -1;
}

int
MitFromID (
	XID		id,
	unsigned short	*data_lenp,
	char		**datap)
{
    struct auth	*auth;

    for (auth = mit_auth; auth; auth=auth->next) {
	if (id == auth->id) {
	    *data_lenp = auth->len;
	    *datap = auth->data;
	    return 1;
	}
    }
    return 0;
}

int
MitRemoveCookie (
	unsigned short	data_length,
	char		*data)
{
    struct auth	*auth, *prev;

    prev = 0;
    for (auth = mit_auth; auth; prev = auth, auth=auth->next) {
	if (data_length == auth->len &&
	    memcmp (data, auth->data, data_length) == 0)
 	{
	    if (prev)
		prev->next = auth->next;
	    else
		mit_auth = auth->next;
	    xfree (auth->data);
	    xfree (auth);
	    return 1;
	}
    }
    return 0;
}

#ifdef XCSECURITY

static char cookie[16]; /* 128 bits */

XID
MitGenerateCookie (
    unsigned	data_length,
    char	*data,
    XID		id,
    unsigned	*data_length_return,
    char	**data_return)
{
    int i = 0;
    int status;

    while (data_length--)
    {
	cookie[i++] += *data++;
	if (i >= sizeof (cookie)) i = 0;
    }
    GenerateRandomData(sizeof (cookie), cookie);
    status = MitAddCookie(sizeof (cookie), cookie, id);
    if (!status)
    {
	id = -1;
    }
    else
    {
	*data_return = cookie;
	*data_length_return = sizeof (cookie);
    }
    return id;
}

#endif /* XCSECURITY */
