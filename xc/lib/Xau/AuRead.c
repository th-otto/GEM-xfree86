/* $TOG: AuRead.c /main/8 1998/02/06 14:14:57 kaleb $ */

/*

Copyright 1988, 1998  The Open Group

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
/* $XFree86: xc/lib/Xau/AuRead.c,v 1.3 1999/05/09 10:51:33 dawes Exp $ */

#include <X11/Xauth.h>

#ifdef X_NOT_STDC_ENV
extern void free();
extern char *malloc();
#else
#include <stdlib.h>
#endif

static int
read_short (unsigned short *shortp, FILE *file)
{
    unsigned char   file_short[2];

    if (fread ((char *) file_short, (int) sizeof (file_short), 1, file) != 1)
	return 0;
    *shortp = file_short[0] * 256 + file_short[1];
    return 1;
}

static int
read_counted_string (unsigned short *countp, char **stringp, FILE *file)
{
    unsigned short  len;
    char	    *data;

    if (read_short (&len, file) == 0)
	return 0;
    if (len == 0) {
	data = 0;
    } else {
    	data = malloc ((unsigned) len);
    	if (!data)
	    return 0;
    	if (fread (data, (int) sizeof (char), (int) len, file) != len) {
	    bzero (data, len);
	    free (data);
	    return 0;
    	}
    }
    *stringp = data;
    *countp = len;
    return 1;
}

Xauth *
XauReadAuth (auth_file)
FILE	*auth_file;
{
    Xauth   local;
    Xauth   *ret;

    if (read_short (&local.family, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.address_length, &local.address, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.number_length, &local.number, auth_file) == 0) {
	if (local.address) free (local.address);
	return 0;
    }
    if (read_counted_string (&local.name_length, &local.name, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	return 0;
    }
    if (read_counted_string (&local.data_length, &local.data, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	return 0;
    }
    ret = (Xauth *) malloc (sizeof (Xauth));
    if (!ret) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	if (local.data) {
	    bzero (local.data, local.data_length);
	    free (local.data);
	}
	return 0;
    }
    *ret = local;
    return ret;
}
